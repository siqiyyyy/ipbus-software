#include "uhal/ProtocolInterfaces.hpp"
#include "uhal/performance.hpp"

namespace uhal
{

Buffers::Buffers ( const uint32_t& aMaxSendSize ) try :
		mSendCounter ( 0 ),
					 mReplyCounter ( 0 ),
					 mSendBuffer ( new uint8_t[ aMaxSendSize ] )
		{}
	catch ( const std::exception& aExc )
	{
		pantheios::log_EXCEPTION ( aExc );
		throw uhal::exception ( aExc );
	}


	Buffers::~Buffers()
	{
		PERFORMANCE ( "" ,

					  if ( mSendBuffer )
	{
		delete mSendBuffer;
		mSendBuffer = NULL;
	}
				)
	}


	const uint32_t& Buffers::sendCounter()
	{
		return mSendCounter;
	}
	const uint32_t& Buffers::replyCounter()
	{
		return mReplyCounter;
	}



	uint8_t* Buffers::send ( const uint8_t* aPtr , const uint32_t& aSize )
	{
		PERFORMANCE ( "" ,
					  uint8_t* lStartPtr ( mSendBuffer+mSendCounter );
					  memcpy ( lStartPtr , aPtr , aSize );
					  mSendCounter += aSize;
					)
		return lStartPtr;
	}


	void Buffers::receive ( uint8_t* aPtr , const uint32_t& aSize )
	{
		PERFORMANCE ( "" ,
					  mReplyBuffer.push_back ( std::make_pair ( aPtr , aSize ) );
					  mReplyCounter += aSize;
					)
	}

	void Buffers::add ( const ValHeader& aValMem )
	{
		PERFORMANCE ( "" ,
					  // 					  mValMems.push_back ( aValMem );
					  mValHeaders.push_back ( aValMem );
					)
	}

	void Buffers::add ( const ValWord< uint32_t >& aValMem )
	{
		PERFORMANCE ( "" ,
					  // 					  mValMems.push_back ( aValMem );
					  mUnsignedValWords.push_back ( aValMem );
					)
	}

	void Buffers::add ( const ValWord< int32_t >& aValMem )
	{
		PERFORMANCE ( "" ,
					  // 					  mValMems.push_back ( aValMem );
					  mSignedValWords.push_back ( aValMem );
					)
	}

	void Buffers::add ( const ValVector< uint32_t >& aValMem )
	{
		PERFORMANCE ( "" ,
					  // 					  mValMems.push_back ( aValMem );
					  mUnsignedValVectors.push_back ( aValMem );
					)
	}

	void Buffers::add ( const ValVector< int32_t >& aValMem )
	{
		PERFORMANCE ( "" ,
					  // 					  mValMems.push_back ( aValMem );
					  mSignedValVectors.push_back ( aValMem );
					)
	}

	uint8_t* Buffers::getSendBuffer()
	{
		return mSendBuffer;
	}

	std::deque< std::pair< uint8_t* , uint32_t > >& Buffers::getReplyBuffer()
	{
		return mReplyBuffer;
	}


	void Buffers::validate()
	{
		for ( std::deque< ValHeader >::iterator lIt = mValHeaders.begin() ; lIt != mValHeaders.end() ; ++lIt )
		{
			lIt->valid ( true );
		}

		for ( std::deque< ValWord< uint32_t > >::iterator lIt = mUnsignedValWords.begin() ; lIt != mUnsignedValWords.end() ; ++lIt )
		{
			lIt->valid ( true );
		}

		for ( std::deque< ValWord< int32_t > >::iterator lIt = mSignedValWords.begin() ; lIt != mSignedValWords.end() ; ++lIt )
		{
			lIt->valid ( true );
		}

		for ( std::deque< ValVector< uint32_t > >::iterator lIt = mUnsignedValVectors.begin() ; lIt != mUnsignedValVectors.end() ; ++lIt )
		{
			lIt->valid ( true );
		}

		for ( std::deque< ValVector< int32_t > >::iterator lIt = mSignedValVectors.begin() ; lIt != mSignedValVectors.end() ; ++lIt )
		{
			lIt->valid ( true );
		}
	}




	void Link ( TransportProtocol& aTransportProtocol , PackingProtocol& aPackingProtocol )
	{
		PERFORMANCE ( "" ,
					  aTransportProtocol.mPackingProtocol = &aPackingProtocol;
					  aPackingProtocol.mTransportProtocol = &aTransportProtocol;
					)
	}




TransportProtocol::TransportProtocol ( const uint32_t& aTimeoutPeriod ) try :
		mTimeoutPeriod ( aTimeoutPeriod )
		{}
	catch ( const std::exception& aExc )
	{
		pantheios::log_EXCEPTION ( aExc );
		throw uhal::exception ( aExc );
	}

	TransportProtocol::~TransportProtocol() {}


	void TransportProtocol::setTimeoutPeriod ( const uint32_t& aTimeoutPeriod )
	{
		mTimeoutPeriod = aTimeoutPeriod;
	}

	const uint32_t& TransportProtocol::getTimeoutPeriod()
	{
		return mTimeoutPeriod;
	}




PackingProtocol::PackingProtocol ( const uint32_t& aMaxSendSize , const uint32_t& aMaxReplySize ) try :
		mTransportProtocol ( NULL ),
						   mCurrentBuffers ( NULL ),
						   mMaxSendSize ( aMaxSendSize ),
						   mMaxReplySize ( aMaxReplySize )
		{}
	catch ( const std::exception& aExc )
	{
		pantheios::log_EXCEPTION ( aExc );
		throw uhal::exception ( aExc );
	}

	PackingProtocol::~PackingProtocol()
	{
		PERFORMANCE ( "" ,

					  if ( mCurrentBuffers )
	{
		delete mCurrentBuffers;
		mCurrentBuffers = NULL;
	}
				)
	}


	void PackingProtocol::Preamble( )
	{
		PERFORMANCE ( "" ,

					  try
		{
			this->ByteOrderTransaction();
		}
		catch ( const std::exception& aExc )
	{
		pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
					)
	}

	void PackingProtocol::Predispatch( )
	{}


	void PackingProtocol::Dispatch( )
	{
		PERFORMANCE ( "" ,

					  if ( mCurrentBuffers )
	{
		this->Predispatch();
			mTransportProtocol->Dispatch ( mCurrentBuffers );
			mCurrentBuffers = NULL;
			mTransportProtocol->Flush();
		}
					)
	}


	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
	// NOTE! THIS FUNCTION MUST BE THREAD SAFE: THAT IS:
	// IT MUST ONLY USE LOCAL VARIABLES
	//            --- OR ---
	// IT MUST MUTEX PROTECT ACCESS TO MEMBER VARIABLES!
	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool PackingProtocol::Validate ( uint8_t* aSendBufferStart ,
									 uint8_t* aSendBufferEnd ,
									 std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyStartIt ,
									 std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyEndIt )
	{
		eIPbusTransactionType lSendIPbusTransactionType , lReplyIPbusTransactionType;
		uint32_t lSendWordCount , lReplyWordCount;
		uint32_t lSendTransactionId , lReplyTransactionId;
		uint8_t lSendResponseGood , lReplyResponseGood;

		do
		{
			if ( ! this->extractIPbusHeader ( * ( ( uint32_t* ) ( aSendBufferStart ) ) , lSendIPbusTransactionType , lSendWordCount , lSendTransactionId , lSendResponseGood ) )
			{
				pantheios::log_ERROR ( "Unable to parse send header " , pantheios::integer ( * ( ( uint32_t* ) ( aSendBufferStart ) ) , pantheios::fmt::fullHex | 10 ) );
				return false;
			}

			if ( ! this->extractIPbusHeader ( * ( ( uint32_t* ) ( aReplyStartIt->first ) ) , lReplyIPbusTransactionType , lReplyWordCount , lReplyTransactionId , lReplyResponseGood ) )
			{
				pantheios::log_ERROR ( "Unable to parse reply header " , pantheios::integer ( * ( ( uint32_t* ) ( aReplyStartIt->first ) ) , pantheios::fmt::fullHex | 10 ) );
				return false;
			}

			if ( lReplyResponseGood )
			{
				pantheios::log_ERROR ( "Returned Response " , pantheios::integer ( lReplyResponseGood , pantheios::fmt::fullHex | 4 ) , " indicated error" );
				return false;
			}

			if ( lSendIPbusTransactionType != lReplyIPbusTransactionType )
			{
				pantheios::log_ERROR ( "Returned Transaction Type " , pantheios::integer ( ( uint8_t ) ( lReplyIPbusTransactionType ) , pantheios::fmt::fullHex | 4 ) ,
									   " does not match that sent " , pantheios::integer ( ( uint8_t ) ( lSendIPbusTransactionType ) , pantheios::fmt::fullHex | 4 ) );
				return false;
			}

			if ( lSendTransactionId != lReplyTransactionId )
			{
				pantheios::log_ERROR ( "Returned Transaction Id " , pantheios::integer ( lReplyTransactionId , pantheios::fmt::fullHex | 10 ) ,
									   " does not match that sent " , pantheios::integer ( lSendTransactionId , pantheios::fmt::fullHex | 10 ) );
				return false;
			}

			switch ( lSendIPbusTransactionType )
			{
				case B_O_T:
				case R_A_I:
					aSendBufferStart += ( 1<<2 );
					break;
				case NI_READ:
				case READ:
					aSendBufferStart += ( 2<<2 );
					break;
				case NI_WRITE:
				case WRITE:
					aSendBufferStart += ( ( 2+lSendWordCount ) <<2 );
					break;
				case RMW_SUM:
					aSendBufferStart += ( 3<<2 );
					break;
				case RMW_BITS:
					aSendBufferStart += ( 4<<2 );
					break;
			}

			switch ( lReplyIPbusTransactionType )
			{
				case B_O_T:
				case NI_WRITE:
				case WRITE:
					aReplyStartIt++;
					break;
				case R_A_I:
				case NI_READ:
				case READ:
				case RMW_SUM:
				case RMW_BITS:
					aReplyStartIt+=2;
					break;
			}
		}
		while ( ( aSendBufferEnd - aSendBufferStart != 0 ) && ( aReplyEndIt - aReplyStartIt != 0 ) );

		return true;
	}
	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------




	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
	// NOTE! THIS FUNCTION MUST BE THREAD SAFE: THAT IS:
	// IT MUST ONLY USE LOCAL VARIABLES
	//            --- OR ---
	// IT MUST MUTEX PROTECT ACCESS TO MEMBER VARIABLES!
	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool PackingProtocol::Validate ( Buffers* aBuffers )
	{
		bool lRet = this->Validate ( aBuffers->getSendBuffer() ,
									 aBuffers->getSendBuffer() +aBuffers->sendCounter() ,
									 aBuffers->getReplyBuffer().begin() ,
									 aBuffers->getReplyBuffer().end() );

		if ( lRet )
		{
			aBuffers->validate();
		}

		return lRet;
	}
	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------




	void PackingProtocol::ByteOrderTransaction()
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus send packet format is:
						  // HEADER
						  uint32_t lSendByteCount ( 1 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  uint32_t lReplyByteCount ( 1 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( B_O_T , 0 ) );
						  ValHeader lReply;
						  mCurrentBuffers->add ( lReply );
						  mCurrentBuffers->receive ( lReply.mMembers->IPbusHeader );
						)
			//return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}

	void PackingProtocol::write ( const uint32_t& aAddr, const uint32_t& aSource )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  // WORD
						  uint32_t lSendByteCount ( 3 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  uint32_t lReplyByteCount ( 1 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( WRITE , 1 ) );
						  mCurrentBuffers->send ( aAddr );
						  mCurrentBuffers->send ( aSource );
						  ValHeader lReply;
						  mCurrentBuffers->add ( lReply );
						  mCurrentBuffers->receive ( lReply.mMembers->IPbusHeader );
						)
			//return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}

	void PackingProtocol::writeBlock ( const uint32_t& aAddr, const std::vector< uint32_t >& aSource, const defs::BlockReadWriteMode& aMode )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  // WORD
						  // WORD
						  // ....
						  uint32_t lSendHeaderByteCount ( 2 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  uint32_t lReplyByteCount ( 1 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  eIPbusTransactionType lType ( ( aMode == defs::INCREMENTAL ) ? WRITE : NI_WRITE );
						  int32_t lPayloadByteCount ( aSource.size() << 2 );
						  uint8_t* lSourcePtr ( ( uint8_t* ) ( & ( aSource.at ( 0 ) ) ) );
						  uint32_t lAddr ( aAddr );

						  while ( lPayloadByteCount > 0 )
		{
			this->checkBufferSpace ( lSendHeaderByteCount+lPayloadByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
				uint32_t lSendBytesAvailableForPayload ( ( lSendBytesAvailable - lSendHeaderByteCount ) & 0xFFFFFFFC );
				//pantheios::log_NOTICE( "lSendBytesAvailable = " , pantheios::integer(lSendBytesAvailable) );
				//pantheios::log_NOTICE( "lSendBytesAvailableForPayload (bytes) = " , pantheios::integer(lSendBytesAvailableForPayload) );
				//pantheios::log_NOTICE( "lSendBytesAvailableForPayload (words) = " , pantheios::integer(lSendBytesAvailableForPayload>>2) );
				//pantheios::log_NOTICE( "lPayloadByteCount = " , pantheios::integer(lPayloadByteCount) );
				mCurrentBuffers->send ( this->calculateIPbusHeader ( lType , lSendBytesAvailableForPayload>>2 ) );
				mCurrentBuffers->send ( lAddr );
				mCurrentBuffers->send ( lSourcePtr , lSendBytesAvailableForPayload );
				lSourcePtr += lSendBytesAvailableForPayload;
				lPayloadByteCount -= lSendBytesAvailableForPayload;

				if ( aMode == defs::INCREMENTAL )
				{
					lAddr += ( lSendBytesAvailableForPayload>>2 );
				}

				ValHeader lReply;
				mCurrentBuffers->add ( lReply );
				mCurrentBuffers->receive ( lReply.mMembers->IPbusHeader );
			}
						)
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	ValWord< uint32_t > PackingProtocol::read ( const uint32_t& aAddr, const uint32_t& aMask )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  uint32_t lSendByteCount ( 2 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  uint32_t lReplyByteCount ( 2 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( READ , 1 ) );
						  mCurrentBuffers->send ( aAddr );
						  ValWord< uint32_t > lReply ( 0 , aMask );
						  mCurrentBuffers->add ( lReply );
						  _ValWord_< uint32_t >& lReplyMem = * ( lReply.mMembers );
						  mCurrentBuffers->receive ( lReplyMem.IPbusHeader );
						  mCurrentBuffers->receive ( lReplyMem.value );
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}

	ValVector< uint32_t > PackingProtocol::readBlock ( const uint32_t& aAddr, const uint32_t& aSize, const defs::BlockReadWriteMode& aMode )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  uint32_t lSendByteCount ( 2 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  // WORD
						  // ....
						  uint32_t lReplyHeaderByteCount ( 1 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  ValVector< uint32_t > lReply ( aSize );
						  _ValVector_< uint32_t >& lReplyMem = * ( lReply.mMembers );
						  uint8_t* lReplyPtr = ( uint8_t* ) ( & ( lReplyMem.value[0] ) );
						  eIPbusTransactionType lType ( ( aMode == defs::INCREMENTAL ) ? READ : NI_READ );
						  int32_t lPayloadByteCount ( aSize << 2 );
						  uint32_t lAddr ( aAddr );

						  while ( lPayloadByteCount > 0 )
		{
			this->checkBufferSpace ( lSendByteCount , lReplyHeaderByteCount+lPayloadByteCount , lSendBytesAvailable , lReplyBytesAvailable );
				uint32_t lReplyBytesAvailableForPayload ( ( lReplyBytesAvailable - lReplyHeaderByteCount ) & 0xFFFFFFFC );
				mCurrentBuffers->send ( this->calculateIPbusHeader ( lType , lReplyBytesAvailableForPayload>>2 ) );
				mCurrentBuffers->send ( lAddr );
				lReplyMem.IPbusHeaders.push_back ( 0 );
				mCurrentBuffers->receive ( lReplyMem.IPbusHeaders.back() );
				mCurrentBuffers->receive ( lReplyPtr , lReplyBytesAvailableForPayload );
				lReplyPtr += lReplyBytesAvailableForPayload;
				lPayloadByteCount -= lReplyBytesAvailableForPayload;

				if ( aMode == defs::INCREMENTAL )
				{
					lAddr += ( lReplyBytesAvailableForPayload>>2 );
				}
			}
			mCurrentBuffers->add ( lReply ); //we store the valmem in the last chunk so that, if the reply is split over many chunks, the valmem is guaranteed to still exist when the other chunks come back...
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ValWord< int32_t > PackingProtocol::readSigned ( const uint32_t& aAddr, const uint32_t& aMask )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  uint32_t lSendByteCount ( 2 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  uint32_t lReplyByteCount ( 2 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( READ , 1 ) );
						  mCurrentBuffers->send ( aAddr );
						  ValWord< int32_t > lReply ( 0 , aMask );
						  mCurrentBuffers->add ( lReply );
						  _ValWord_< int32_t >& lReplyMem = * ( lReply.mMembers );
						  mCurrentBuffers->receive ( lReplyMem.IPbusHeader );
						  mCurrentBuffers->receive ( lReplyMem.value );
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}

	ValVector< int32_t > PackingProtocol::readBlockSigned ( const uint32_t& aAddr, const uint32_t& aSize, const defs::BlockReadWriteMode& aMode )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  uint32_t lSendByteCount ( 2 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  // WORD
						  // ....
						  uint32_t lReplyHeaderByteCount ( 1 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  ValVector< int32_t > lReply ( aSize );
						  _ValVector_< int32_t >& lReplyMem = * ( lReply.mMembers );
						  uint8_t* lReplyPtr = ( uint8_t* ) ( & ( lReplyMem.value[0] ) );
						  eIPbusTransactionType lType ( ( aMode == defs::INCREMENTAL ) ? READ : NI_READ );
						  int32_t lPayloadByteCount ( aSize << 2 );
						  uint32_t lAddr ( aAddr );

						  while ( lPayloadByteCount > 0 )
		{
			this->checkBufferSpace ( lSendByteCount , lReplyHeaderByteCount+lPayloadByteCount , lSendBytesAvailable , lReplyBytesAvailable );
				uint32_t lReplyBytesAvailableForPayload ( ( lReplyBytesAvailable - lReplyHeaderByteCount ) & 0xFFFFFFFC );
				mCurrentBuffers->send ( this->calculateIPbusHeader ( lType , lReplyBytesAvailableForPayload>>2 ) );
				mCurrentBuffers->send ( lAddr );
				lReplyMem.IPbusHeaders.push_back ( 0 );
				mCurrentBuffers->receive ( lReplyMem.IPbusHeaders.back() );
				mCurrentBuffers->receive ( lReplyPtr , lReplyBytesAvailableForPayload );
				lReplyPtr += lReplyBytesAvailableForPayload;
				lPayloadByteCount -= lReplyBytesAvailableForPayload;

				if ( aMode == defs::INCREMENTAL )
				{
					lAddr += ( lReplyBytesAvailableForPayload>>2 );
				}
			}
			mCurrentBuffers->add ( lReply ); //we store the valmem in the last chunk so that, if the reply is split over many chunks, the valmem is guaranteed to still exist when the other chunks come back...
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ValVector< uint32_t > PackingProtocol::readReservedAddressInfo ()
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  uint32_t lSendByteCount ( 1 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  // WORD
						  uint32_t lReplyByteCount ( 3 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( R_A_I , 0 ) );
						  ValVector< uint32_t > lReply ( 2 );
						  mCurrentBuffers->add ( lReply );
						  _ValVector_< uint32_t >& lReplyMem = * ( lReply.mMembers );
						  lReplyMem.IPbusHeaders.push_back ( 0 );
						  mCurrentBuffers->receive ( lReplyMem.IPbusHeaders[0] );
						  mCurrentBuffers->receive ( ( uint8_t* ) ( & ( lReplyMem.value[0] ) ) , 2<<2 );
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ValWord< uint32_t > PackingProtocol::rmw_bits ( const uint32_t& aAddr , const uint32_t& aANDterm , const uint32_t& aORterm )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  // AND TERM
						  // OR TERM
						  uint32_t lSendByteCount ( 4 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  uint32_t lReplyByteCount ( 2 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( RMW_BITS , 1 ) );
						  mCurrentBuffers->send ( aAddr );
						  mCurrentBuffers->send ( aANDterm );
						  mCurrentBuffers->send ( aORterm );
						  ValWord< uint32_t > lReply ( 0 );
						  mCurrentBuffers->add ( lReply );
						  _ValWord_< uint32_t >& lReplyMem = * ( lReply.mMembers );
						  mCurrentBuffers->receive ( lReplyMem.IPbusHeader );
						  mCurrentBuffers->receive ( lReplyMem.value );
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ValWord< int32_t > PackingProtocol::rmw_sum ( const uint32_t& aAddr , const int32_t& aAddend )
	{
		try
		{
			PERFORMANCE ( "" ,
						  // IPbus packet format is:
						  // HEADER
						  // BASE ADDRESS
						  // ADDEND
						  uint32_t lSendByteCount ( 3 << 2 );
						  // IPbus reply packet format is:
						  // HEADER
						  // WORD
						  uint32_t lReplyByteCount ( 2 << 2 );
						  uint32_t lSendBytesAvailable;
						  uint32_t  lReplyBytesAvailable;
						  this->checkBufferSpace ( lSendByteCount , lReplyByteCount , lSendBytesAvailable , lReplyBytesAvailable );
						  mCurrentBuffers->send ( this->calculateIPbusHeader ( RMW_SUM , 1 ) );
						  mCurrentBuffers->send ( aAddr );
						  mCurrentBuffers->send ( static_cast< uint32_t > ( aAddend ) );
						  ValWord< int32_t > lReply ( 0 );
						  mCurrentBuffers->add ( lReply );
						  _ValWord_< int32_t >& lReplyMem = * ( lReply.mMembers );
						  mCurrentBuffers->receive ( lReplyMem.IPbusHeader );
						  mCurrentBuffers->receive ( lReplyMem.value );
						)
			return lReply;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void PackingProtocol::checkBufferSpace ( const uint32_t& aRequestedSendSize , const uint32_t& aRequestedReplySize , uint32_t& aAvailableSendSize , uint32_t& aAvailableReplySize )
	{
		try
		{
			if ( !mCurrentBuffers )
			{
				mCurrentBuffers = new Buffers ( mMaxSendSize );
				this->Preamble();
			}

			uint32_t lSendBufferFreeSpace ( mMaxSendSize - mCurrentBuffers->sendCounter() );
			uint32_t lReplyBufferFreeSpace ( mMaxReplySize - mCurrentBuffers->replyCounter() );

			if ( ( aRequestedSendSize <= lSendBufferFreeSpace ) && ( aRequestedReplySize <= lReplyBufferFreeSpace ) )
			{
				aAvailableSendSize = aRequestedSendSize;
				aAvailableReplySize = aRequestedReplySize;
				return;
			}

			if ( ( lSendBufferFreeSpace > 16 ) && ( lReplyBufferFreeSpace > 16 ) )
			{
				aAvailableSendSize = lSendBufferFreeSpace;
				aAvailableReplySize = lReplyBufferFreeSpace;
				return;
			}

			this->Predispatch();
			mTransportProtocol->Dispatch ( mCurrentBuffers );
			mCurrentBuffers = new Buffers ( mMaxSendSize );
			this->Preamble();
			lSendBufferFreeSpace = mMaxSendSize - mCurrentBuffers->sendCounter();
			lReplyBufferFreeSpace = mMaxReplySize - mCurrentBuffers->replyCounter();

			if ( ( aRequestedSendSize <= lSendBufferFreeSpace ) && ( aRequestedReplySize <= lReplyBufferFreeSpace ) )
			{
				aAvailableSendSize = aRequestedSendSize;
				aAvailableReplySize = aRequestedReplySize;
				return;
			}

			aAvailableSendSize = lSendBufferFreeSpace;
			aAvailableReplySize = lReplyBufferFreeSpace;
		}
		catch ( const std::exception& aExc )
		{
			pantheios::log_EXCEPTION ( aExc );
			throw uhal::exception ( aExc );
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

}

