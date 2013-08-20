%%% ===========================================================================
%%% @author Robert Frazier
%%% @author Tom Williams
%%%
%%% @since May 2012
%%%
%%% @doc A Transaction Manager is a process that accepts an incoming TCP
%%%      connection from a (microHAL) client, and then manages requests from
%%%      it for the lifetime of the connection.
%%% @end
%%% ===========================================================================
-module(ch_transaction_manager).

%%
%% Include files
%%
-include("ch_global.hrl").
-include("ch_timeouts.hrl").
-include("ch_error_codes.hrl").

%%
%% Exported Functions
%%
-export([start_link/1, tcp_acceptor/1]).

%%
%% API Functions
%%

%% ---------------------------------------------------------------------
%% @doc Starts up a transaction manager process that will wait for a
%%      client to connect on the provided TCP socket. On acceptance of
%%      a connection (or an accept error), the function: 
%%        ch_tcp_listener:connection_accept_completed()
%%      is called.  The transaction manager process reaches end of life
%%      when the TCP connection is terminated by the microHAL client.
%%
%% @spec start_link(TcpListenSocket::socket) -> ok
%% @end
%% ---------------------------------------------------------------------
start_link(TcpListenSocket) ->
    ?CH_LOG_DEBUG("Spawning new transaction manager."),
    proc_lib:spawn_link(?MODULE, tcp_acceptor, [TcpListenSocket]),
    ok.
    

%% Blocks until a client connection is accepted on the given socket.
%% This function is really "private", and should only be called via
%% the start_link function above.  Need to find a way of refactoring
%% this module so it doesn't need to be an external function.
tcp_acceptor(TcpListenSocket) ->
    ?CH_LOG_DEBUG("Transaction manager born. Waiting for TCP connection..."),
    case gen_tcp:accept(TcpListenSocket) of
        {ok, ClientSocket} ->
            ch_tcp_listener:connection_accept_completed(),
            ch_stats:client_connected(),
            case inet:peername(ClientSocket) of
                {ok, {_ClientAddr, _ClientPort}} ->
                    TcpPid = proc_lib:spawn_link(fun tcp_proc_init/0),
                    put(tcp_pid, TcpPid),
                    gen_tcp:controlling_process(ClientSocket, TcpPid),
                    TcpPid ! {start, ClientSocket, self()},
                    ?CH_LOG_INFO("TCP socket accepted from client at IP addr=~w, port=~p~n"
                                 "Socket options: ~w~n",
                                 [_ClientAddr, _ClientPort, inet:getopts(TcpListenSocket, [active, buffer, low_watermark, high_watermark, recbuf, sndbuf])]),
                    minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, unknown, unknown, unknown, 1, queue:new(), []);
                _Else ->
                    ch_stats:client_disconnected(),
                    ?CH_LOG_ERROR("Socket error whilst getting peername.")
            end;
        {error, _Reason} ->
            ch_tcp_listener:connection_accept_completed(),
            % Something funny happened whilst trying to accept the socket.
            ?CH_LOG_ERROR("Error (~p) occurred during TCP accept.", [_Reason]);
        _Else ->  % Belt and braces...
            ch_tcp_listener:connection_accept_completed(),
            ?CH_LOG_ERROR("Unexpected event (~p) occurred during TCP accept.", [_Else])
    end,
    ?CH_LOG_INFO("I am now redundant; exiting normally."),
    ok.



%%% --------------------------------------------------------------------
%%% Internal functions
%%% --------------------------------------------------------------------


% initialisation function for TCP receive/send process
tcp_proc_init() ->
    receive
        {start, Socket, ParentPid} ->
            link(ParentPid),
            tcp_proc_loop(Socket, ParentPid)
    end.


%% Tail-recursive loop function for TCP receive/send process
tcp_proc_loop(Socket, ParentPid) ->
    % Give priority to receiving packets off TCP stream (wrt sending back to uHAL client)
    receive
        {tcp, Socket, Packet} ->
            ParentPid ! {tcp, Socket, Packet}
    after 0 ->
        receive
            {send, Pkt} ->
                true = erlang:port_command(Socket, Pkt, []);
            {inet_reply, Socket, ok} ->
                void;
            {inet_reply, Socket, SendError} ->
                ?CH_LOG_ERROR("Error in TCP async send: ~w", [SendError]),
                throw({tcp_send_error,SendError});
            Other ->
                ParentPid ! Other
        end
    end,
    tcp_proc_loop(Socket, ParentPid).


%% Main transaction_manager operation loop
%%   - entered when accept off TCP socket
%%   - if socket closes, then this function exits
minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPAddrArg, TargetPortArg, DevClientPid, NrInFlight, NrReqsQ, ReplyIoList) ->
    receive
        {tcp, ClientSocket, RequestBin} ->
            ?CH_LOG_DEBUG("Received TCP chunk."),
            {RetPid, TargetIPU32, TargetPort, NrReqs} = unpack_and_enqueue(RequestBin, DevClientPid, 0),
            minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPU32, TargetPort, RetPid, NrInFlight+NrReqs, queue:in(NrReqs, NrReqsQ), ReplyIoList);

        {device_client_response, TargetIPAddrArg, TargetPortArg, 0, ReplyPkt} ->
            ErrorCode = 0,
            NrRepliesToSend = element(2, queue:peek(NrReqsQ)),
            case ((lists:flatlength(ReplyIoList) + 2) div 2) of
                NrRepliesToSend ->
                    ?CH_LOG_DEBUG("Sending ~w IPbus response packets over TCP.", []),
                    TcpPid ! {send, [ReplyIoList, <<(byte_size(ReplyPkt) + 8):32, TargetIPAddrArg:32, TargetPortArg:16, ErrorCode:16>>, ReplyPkt]},
                    minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPAddrArg, TargetPortArg, DevClientPid, NrInFlight-1, queue:drop(NrReqsQ), []);
                _NrRepliesAccumulated ->
                    ?CH_LOG_DEBUG("IPbus response packet received. Accumulating for TCP send (~w needed for next TCP chunk, ~w now accumulated).", [NrRepliesToSend, _NrRepliesAccumulated]),
                    minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPAddrArg, TargetPortArg, DevClientPid, NrInFlight-1, NrReqsQ, 
                                                     [ReplyIoList, <<(byte_size(ReplyPkt) + 8):32, TargetIPAddrArg:32, TargetPortArg:16, ErrorCode:16>>, ReplyPkt])
            end;

        {device_client_response, TargetIPAddrArg, TargetPortArg, ErrorCode, Bin} ->
            ?CH_LOG_DEBUG("Received IPbus response with ErrorCode ~w. Sending directly onto TCP stream."),
            TcpPid ! {send, [<<(byte_size(Bin) + 8):32, TargetIPAddrArg:32, TargetPortArg:16, ErrorCode:16>>, Bin]},
            minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPAddrArg, TargetPortArg, unknown, NrInFlight-1, NrReqsQ, ReplyIoList);

        {tcp_closed, ClientSocket} ->
            ch_stats:client_disconnected(),
            ?CH_LOG_INFO("TCP socket closed.");

        {tcp_error, ClientSocket, _Reason} ->
            % Assume this ends up with the socket closed from a stats standpoint
            ch_stats:client_disconnected(),
            ?CH_LOG_WARN("TCP socket error (~p).", [_Reason]);

        Else ->
            ?CH_LOG_WARN("Received and ignoring unexpected message: ~p", [Else]),
            minimal_tcp_receive_handler_loop(TcpPid, ClientSocket, TargetIPAddrArg, TargetPortArg, DevClientPid, NrInFlight, NrReqsQ, ReplyIoList)
    end.

            
unpack_and_enqueue(<<TargetIPAddr:32, TargetPort:16, NrInstructions:16, IPbusReq:NrInstructions/binary-unit:32, Tail/binary>>, Pid, NrSent) ->
    RetPid = enqueue_request(TargetIPAddr, TargetPort, Pid, IPbusReq),
    if
      byte_size(Tail) > 0 ->
        unpack_and_enqueue(Tail, RetPid, NrSent+1);
      true ->
        ?CH_LOG_DEBUG("~w IPbus packets in last TCP chunk", [(NrSent+1)]),
        {RetPid, TargetIPAddr, TargetPort, NrSent+1}
    end.


enqueue_request(_IPaddrU32, _PortU16, DestPid, IPbusRequest) when is_pid(DestPid) ->
    ?CH_LOG_DEBUG("Enqueueing IPbus request packet for ~w to PID ~w", [ch_utils:ip_port_string(_IPaddrU32,_PortU16), DestPid]),
    gen_server:cast(DestPid, {send, IPbusRequest, self()}),
    DestPid;
enqueue_request(IPaddrU32, PortU16, _NotPid, IPbusRequest) ->
    {ok, Pid} = ch_device_client_registry:get_pid(IPaddrU32, PortU16),
    enqueue_request(IPaddrU32, PortU16, Pid, IPbusRequest).

