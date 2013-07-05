#!/bin/env escript
%% -*- erlang -*-
%%! -C -pa ebin/unittest +zdbbl 2097151



% ---------------------------------------------------------------------------------------
%    MACROS / RECORDS
% ---------------------------------------------------------------------------------------

%-define(UDP_SOCKET_OPTIONS, [binary, {active, true}, {buffer, 100000}, {recbuf, 100000}, {read_packets, 50}]). % {active, false}
-define(UDP_SOCKET_OPTIONS, [binary, {active, true}]).

-define(TCP_SOCKET_OPTIONS, [binary, {packet, 4}, {nodelay, true}, {active, true}, {backlog, 256}, {buffer, 100000}, {low_watermark, 50000}, {high_watermark, 100000}]).
%-define(TCP_SOCKET_OPTIONS, [binary, {packet, 4}, {nodelay, true}, {active, true}, {backlog, 256}]).


% ---------------------------------------------------------------------------------------
%    MAIN, USAGE & OPTION PARSING
% ---------------------------------------------------------------------------------------

usage() ->
    io:format("usage: scripts/perf_tester.escript <runmode> <IP> <port> <nr_iterations> <nr_in_flight>~n~n").


parse_args([ArgIP, ArgPort, ArgItns, ArgInFlight]) ->
    {ok, IP} = inet_parse:address(ArgIP),
    Port = list_to_integer(ArgPort),
    Itns = list_to_integer(ArgItns),
    NrInFlight = list_to_integer(ArgInFlight),
    io:format("Arguments: ~n"
              "   Target              @ ~w:~w~n"
              "   Number of iterations: ~w~n"
              "   Max in flight:        ~w~n",
              [IP, Port, Itns, NrInFlight]),
    {IP, Port, Itns, NrInFlight};
parse_args(_) ->
    io:format("Wrong number of arguments given!"),
    usage(),
    halt(1).

create_udp_socket() ->
    io:format("Creating UDP socket with options: ~w ~n", [?UDP_SOCKET_OPTIONS]),
    {ok, Socket} = gen_udp:open(0, ?UDP_SOCKET_OPTIONS),
    {ok, OptValues} =  inet:getopts(Socket, [active, buffer, delay_send, read_packets, recbuf, sndbuf]),
    io:format("Socket opened! Option values are: ~w~n", [OptValues]),
    Socket.


tcp_connect(TargetIP, TargetPort) ->
    SocketOptions = lists:keydelete(backlog, 1, ?TCP_SOCKET_OPTIONS),
    io:format("Connecting TCP stream to ~w:~w with options: ~w ~n", [TargetIP, TargetPort, SocketOptions]),
    {ok, Socket} = gen_tcp:connect(TargetIP, TargetPort, SocketOptions),
    io:format("Socket opened! Option values are: ~w~n",            
              [inet:getopts(Socket, [active, buffer, delay_send, read_packets, recbuf, sndbuf, low_watermark, high_watermark])]
             ),
    Socket.


print_results(NrItns, MicroSecs, ReqBytes, ReplyBytes) ->
    BytesSent = NrItns * ReqBytes,
    BytesRecd = NrItns * ReplyBytes,
    MicroSecsPerIt = MicroSecs / NrItns,
    MbitPerSecSent = (BytesSent * 8 / 1000000.0) / (MicroSecs / 1000000.0),
    MbitPerSecRecd = (BytesRecd * 8 / 1000000.0) / (MicroSecs / 1000000.0),
    io:format("~nSent/rcvd ~w packets in ~w secs, time per packet = ~wus~n", [NrItns, MicroSecs/1000000.0, MicroSecsPerIt]),
    io:format("   Send B/W = ~w Mb/s~n", [MbitPerSecSent]),
    io:format("   Recv B/W = ~w Mb/s~n", [MbitPerSecRecd]).


main(["udp_ipbus_client" | OtherArgs]) ->
    {TargetIP, TargetPort, NrItns, NrInFlight} = parse_args(OtherArgs),
    Socket = create_udp_socket(),
    %
    Request = <<16#200000f0:32,
                16#2cbaff1f:32,
                0:((16#100)*32),
                16#20005a1f:32,
                0:((15#5b)*32)
              >>,
    Reply = << 16#200000f0:32,
               16#2cbaff10:32,
               16#20005a10:32
            >>,
    {MicroSecs, ok} = timer:tc( fun () -> ch_unittest_common:udp_client_loop(Socket, TargetIP, TargetPort, Request, Reply, {0,NrInFlight}, NrItns) end),
    print_results(NrItns, MicroSecs, byte_size(Request), byte_size(Reply));

main(["udp_echo_client" | OtherArgs]) ->
    {TargetIP, TargetPort, NrItns, NrInFlight} = parse_args(OtherArgs),
    Socket = create_udp_socket(),
    %
    Request = <<16#200000f0:32,
                16#2cbaff1f:32,
                0:((16#100)*32),
                16#20005a1f:32,
                0:((15#5b)*32)
              >>,
    {MicroSecs, ok} = timer:tc( fun () -> ch_unittest_common:udp_client_loop(Socket, TargetIP, TargetPort, Request, Request, {0,NrInFlight}, NrItns) end),
    print_results(NrItns, MicroSecs, byte_size(Request), byte_size(Request));

main(["udp_echo_server"]) ->
    io:format("Starting echo server with options: ~p~n", [?UDP_SOCKET_OPTIONS]),
    ch_unittest_common:start_udp_echo_server(?UDP_SOCKET_OPTIONS);

main(["tcp_echo_client" | OtherArgs]) ->
    {TargetIP, TargetPort, NrItns, NrInFlight} = parse_args(OtherArgs),
    Socket = tcp_connect(TargetIP, TargetPort),
    %
    Request = <<16#200000f0:32,
                16#2cbaff1f:32,
                0:((16#100)*32),
                16#20005a1f:32,
                0:((15#5b)*32)
              >>,
    {ok, [{active, ActiveValue}]} = inet:getopts(Socket, [active]),
    {MicroSecs, ok} = timer:tc( fun () -> ch_unittest_common:tcp_client_loop({Socket, ActiveValue}, Request, Request, {0,NrInFlight}, NrItns) end ),
    gen_tcp:close(Socket),
    print_results(NrItns, MicroSecs, byte_size(Request), byte_size(Request));

main(["tcp_echo_server", ArgPort]) ->
    Port = list_to_integer(ArgPort),
    io:format("Starting TCP echo server with options: ~p~n", [?TCP_SOCKET_OPTIONS]),
    ch_unittest_common:start_tcp_echo_server(?TCP_SOCKET_OPTIONS, Port);
    
main(_) ->
    io:format("Incorrect usage!~n"),
    usage().


