/*
---------------------------------------------------------------------------

    This file is part of uHAL.

    uHAL is a hardware access library and programming framework
    originally developed for upgrades of the Level-1 trigger of the CMS
    experiment at CERN.

    uHAL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uHAL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uHAL.  If not, see <http://www.gnu.org/licenses/>.


      Andrew Rose, Imperial College, London
      email: awr01 <AT> imperial.ac.uk

      Marc Magrans de Abril, CERN
      email: marc.magrans.de.abril <AT> cern.ch

---------------------------------------------------------------------------
*/

// #include "uhal/performance.hpp"
// #include <boost/lambda/bind.hpp>
#include <boost/bind/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>


#include <sys/time.h>

namespace uhal
{


  template < typename InnerProtocol >
  ControlHub< InnerProtocol >::ControlHub ( const std::string& aId, const URI& aUri ) :
    InnerProtocol ( aId , aUri ),
    mDeviceIPaddress ( 0 ),
    mDevicePort ( 0 ),
    mTransactionCounter ( 0 )
  {
    std::pair< uint32_t , uint16_t > lPair ( ExtractTargetID ( aUri ) );
    mDeviceIPaddress = htonl ( lPair.first );
    mDevicePort = htons ( lPair.second );
    //log ( Debug() , ThisLocation() );
  }


  template < typename InnerProtocol >
  ControlHub< InnerProtocol >::~ControlHub()
  {
    //log ( Debug() , ThisLocation() );
  }


  template < typename InnerProtocol >
  void ControlHub< InnerProtocol >::preamble( )
  {
    //log ( Debug() , ThisLocation() );
    // -------------------------------------------------------------------------------------------------------------
    // 12 bytes form the preamble:
    // Byte-count (4 bytes) will be updated before transmission in predispatch
    // Device IP address (4 bytes)
    // Device Port number (2 bytes)
    // Word-count (2 bytes) will be updated before transmission in predispatch
    // -------------------------------------------------------------------------------------------------------------
    // 16 bytes form the preamble reply:
    // Total Byte-count (4 bytes)
    // Chunk Byte-count (4 bytes)
    // Device IP address (4 bytes)
    // Device Port number (2 bytes)
    // Error code (2 bytes)
    // -------------------------------------------------------------------------------------------------------------
    mPreambles.push_back ( tpreamble() );
    tpreamble* lPreambles = & mPreambles.back();
    std::deque < Buffers >::iterator lCurrentFillingBuffers ( this->mCurrentBuffers );
    lPreambles->mSendByteCountPtr = ( uint32_t* ) ( lCurrentFillingBuffers->send ( ( uint32_t ) ( 0 ) ) );
    lCurrentFillingBuffers->send ( mDeviceIPaddress );
    lCurrentFillingBuffers->send ( mDevicePort );
    lPreambles->mSendWordCountPtr = ( uint16_t* ) ( lCurrentFillingBuffers->send ( ( uint16_t ) ( 0 ) ) );
    lCurrentFillingBuffers->receive ( lPreambles->mReplyTotalByteCounter );
    lCurrentFillingBuffers->receive ( lPreambles->mReplyChunkByteCounter );
    lCurrentFillingBuffers->receive ( lPreambles->mReplyDeviceIPaddress );
    lCurrentFillingBuffers->receive ( lPreambles->mReplyDevicePort );
    lCurrentFillingBuffers->receive ( lPreambles->mReplyErrorCode );
    InnerProtocol::preamble();
  }



  template < typename InnerProtocol >
  uint32_t ControlHub< InnerProtocol >::getPreambleSize()
  {
    return InnerProtocol::getPreambleSize() +3;
  }


  template < typename InnerProtocol >
  void ControlHub< InnerProtocol >::predispatch( )
  {
    //log ( Debug() , ThisLocation() );
    tpreamble& lPreambles = mPreambles.back();
    uint32_t lByteCount ( this->mCurrentBuffers->sendCounter() );
    * ( lPreambles.mSendByteCountPtr ) = htonl ( lByteCount-4 );
    * ( lPreambles.mSendWordCountPtr ) = htons ( ( lByteCount-12 ) >>2 );

    InnerProtocol::predispatch();
  }


  template < typename InnerProtocol >
  bool ControlHub< InnerProtocol >::validate ( uint8_t* aSendBufferStart ,
      uint8_t* aSendBufferEnd ,
      std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyStartIt ,
      std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyEndIt )
  {
    aReplyStartIt++;
    aReplyStartIt++;
    uint32_t lReplyIPaddress ( * ( ( uint32_t* ) ( aReplyStartIt->first ) ) );

    if ( lReplyIPaddress != mDeviceIPaddress )
    {
      log ( Error() , "Returned IP address " , Integer ( lReplyIPaddress , IntFmt< hex , fixed >() ) ,
            " does not match that sent " , Integer ( mDeviceIPaddress, IntFmt< hex , fixed >() ) );
      mPreambles.pop_front();
      return false;
    }

    aReplyStartIt++;
    uint16_t lReplyPort ( * ( ( uint16_t* ) ( aReplyStartIt->first ) ) );

    if ( lReplyPort != mDevicePort )
    {
      log ( Error() , "Returned Port number " , Integer ( lReplyPort ) ,
            " does not match that sent " , Integer ( mDevicePort ) );
      mPreambles.pop_front();
      return false;
    }

    aReplyStartIt++;
    uint16_t lErrorCode ( * ( ( uint16_t* ) ( aReplyStartIt->first ) ) );

    if ( lErrorCode != 0 )
    {
      log ( Error() , "Control Hub reported error code " , Integer ( lErrorCode, IntFmt< hex , fixed >() ) );
      mPreambles.pop_front();
      return false;
    }

    //aReplyStartIt++;
    mPreambles.pop_front();
    // log ( Info() , "Control Hub has validated the packet headers" );
    return InnerProtocol::validate ( ( aSendBufferStart+=12 ) , aSendBufferEnd , ( ++aReplyStartIt ) , aReplyEndIt );
  }





  template < typename InnerProtocol >
  bool ControlHub< InnerProtocol >::validate()
  {
    return ClientInterface::validate();
  }


  //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

}

