#include "uhal/grammars/URLGrammar.hpp"

#include <boost/spirit/include/qi.hpp>

#include "uhal/log/log.hpp"

std::ostream& operator<< ( std::ostream& aStream , const uhal::URI& aURI )
{
  aStream << " > protocol : " << aURI.mProtocol << "\n";
  aStream << " > hostname : " << aURI.mHostname << "\n";
  aStream << " > port : " << aURI.mPort << "\n";
  aStream << " > path : " << aURI.mPath << "\n";
  aStream << " > extension : " << aURI.mExtension << "\n";
  aStream << " > arguments :\n";

  for ( uhal::NameValuePairVectorType::const_iterator lIt = aURI.mArguments.begin() ; lIt != aURI.mArguments.end() ; ++lIt )
  {
    aStream << "   > " << lIt->first << " = " << lIt->second << "\n";
  }

  aStream << std::flush;
  return aStream;
}



namespace uhal
{
  /**
  	The log_inserter function to add a URI object to a log entry
  	@param aURI a URI object to format and print to log
  */
  template < >
  void log_inserter< URI > ( const URI& aURI )
  {
    log_inserter ( " > protocol : " );
    log_inserter ( aURI.mProtocol );
    log_inserter ( "\n > hostname : " );
    log_inserter ( aURI.mHostname );
    log_inserter ( "\n > port : " );
    log_inserter ( aURI.mPort );
    log_inserter ( "\n > path : " );
    log_inserter ( aURI.mPath );
    log_inserter ( "\n > extension : " );
    log_inserter ( aURI.mExtension );
    log_inserter ( "\n > arguments :\n" );

    for ( NameValuePairVectorType::const_iterator lIt = aURI.mArguments.begin() ; lIt != aURI.mArguments.end() ; ++lIt )
    {
      log_inserter ( "   > " );
      log_inserter ( lIt->first.data() );
      log_inserter ( " = " );
      log_inserter ( lIt->second.data() );
      log_inserter ( '\n' );
    }
  }
}



namespace grammars
{
  URIGrammar::URIGrammar() :
    URIGrammar::base_type ( start )
  {
    using namespace boost::spirit;
    start = protocol > hostname > port > - ( path ) > - ( extension ) > - ( data_pairs_vector );
    protocol = + ( qi::char_ - qi::lit ( ":" ) ) > qi::lit ( "://" );
    hostname = + ( qi::char_ - qi::lit ( ":" ) ) > qi::lit ( ":" );
    port 	 = + ( qi::char_ - ascii::punct ) ;
    path 				= qi::lit ( "/" ) > + ( qi::char_ - qi::lit ( "." ) - qi::lit ( "?" ) );
    extension 			= qi::lit ( "." ) > + ( qi::char_ - qi::lit ( "?" ) ) ;
    data_pairs_vector 	= qi::lit ( "?" ) > *data_pairs;
    data_pairs = data_pairs_1 > data_pairs_2;
    data_pairs_1 = + ( qi::char_ - qi::lit ( "=" ) ) > qi::lit ( "=" );
    data_pairs_2 = * ( qi::char_ - qi::lit ( "&" ) ) >> - ( qi::lit ( "&" ) );
  }

}

