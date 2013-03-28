#include <fstream>
#include <iostream>
#include <string>
#include <queue>

#include <cstdlib>

#define DIGITCONST 60
#define DIGITMAX  100

// (c) Shirokov Mikhail, 2013.

//*** TimeModifer **************************************************************

class TimeModifer
{
	int _msec;
	
	std::size_t digit( char d ) const	// throw std::string
	{
		if ( d >= '0' && d <= '9' )
			return d - '0';
		throw std::string( "gotten \'" ) + d + "\' is not digit";
	}
	
	std::size_t strToSize_t( const std::string& toConvert ) const
	{
		try
		{
			std::size_t result = 0;
			for ( std::size_t i = 0; i < toConvert.size(); ++i )
			{
				result = result * 10;
				result += digit( toConvert[i] );
				//FIXME!!! sizeof( size_t )
			}
			return result;
		}
		catch( std::string& e )
		{
			throw e + " in string: " + toConvert;
		}
	}
	
	std::string strFromDigit( size_t d ) const	// throw std::string
	{
		if ( !( d >= 0 && d <= 9 ) )
			throw std::string( "can not convert digit to string" );
		char s = '0' + d;
		return ( std::string() += s );
	}
	
public:
	TimeModifer() {}
	~TimeModifer() {}
	
	void init( const std::string& timeSrt )
	{
		try
		{
			int hrs = strToSize_t( timeSrt.substr( 0, 2 ) );
			int min = strToSize_t( timeSrt.substr( 3, 2 ) );
			int sec = strToSize_t( timeSrt.substr( 6, 2 ) );
			int msec = strToSize_t( timeSrt.substr( 9, 3 ) );
			
			_msec = msec +
					sec * 1000 +
					min * 1000 * 60 +
					hrs * 1000 * 60 * 60;
		}
		catch( std::string& e )
		{
			throw std::string( "time init() error: " ) + e;
		}
	}
	
	void add( int msec ) { _msec += msec; }
	
	bool isCorrect() const { return _msec >= 0; }
	
	std::string getTime() const
	{
		if ( !isCorrect() )
			return std::string ( "00:00:00,000" );
			
		std::string timeStr;
		
		std::size_t part = _msec;
		
		std::size_t hrs = part / ( 1000 * 60 * 60 );
		part = part % ( 1000 * 60 * 60 );
		timeStr += strFromDigit( hrs / 10 );
		timeStr += strFromDigit( hrs % 10 ) + ':';
		
		std::size_t min = part / ( 1000 * 60 );
		part = part % ( 1000 * 60 );
		timeStr += strFromDigit( min / 10 );
		timeStr += strFromDigit( min % 10 ) + ':';
		
		std::size_t sec = part / 1000;
		part = part % 1000;
		timeStr += strFromDigit( sec / 10 );
		timeStr += strFromDigit( sec % 10 ) + ',';
		
		timeStr += strFromDigit( part / 100 );
		timeStr += strFromDigit( ( part / 10 ) % 10 );
		timeStr += strFromDigit( part % 10 );
		
		return timeStr;
	}
	
	void addSec( int sec ) { add( sec * 1000 ); }
};

//*** subWorker ****************************************************************

class subWorker
{
protected:
    std::ifstream _file;

private:
    virtual void readBlock() = 0;
    virtual void writeBlock( std::ofstream& out ) = 0;

public:
    subWorker()             {}
    virtual ~subWorker()    { _file.close(); }

    virtual bool init( const std::string& _path )
    {
        if ( _file.good() ) _file.close();
        _file.open( _path.c_str(), std::ifstream::in );
        return _file.good();
    }

    virtual void doShift( std::string pathOut, int sec ) = 0;
};

//*** subSrt *******************************************************************

class subSrt : public subWorker
{
    int _sec;
	TimeModifer _tModifer;
    std::vector< std::string > _block;

    void timeShift()
    {
		std::string timeLine = _block[1];
		std::string newTimeLine;
		
		// TIME_BEGIN
		std::size_t spacer = timeLine.find( " " );
		_tModifer.init( timeLine.substr( 0, spacer ) );
		_tModifer.addSec( _sec );
		newTimeLine = _tModifer.getTime();
		
		// -->
		++spacer;
		timeLine = timeLine.substr( spacer );
		spacer = timeLine.find( " " );
		newTimeLine += " ";
		newTimeLine += timeLine.substr( 0, spacer );
		newTimeLine += " ";
		
		// TIME_END
		++spacer;
		_tModifer.init( timeLine.substr( spacer ) );
		_tModifer.addSec( _sec );
		newTimeLine += _tModifer.getTime();
		
		// COORDINATES
		spacer = timeLine.find( " ", spacer );
		if ( spacer != std::string::npos )
		{
			newTimeLine += timeLine.substr( spacer );
		}
		
		if ( !_tModifer.isCorrect() ) _block.clear();
		else _block[1] = newTimeLine;
    }
	
	std::string trimLeft( const std::string& str ) const
	{
		if ( str.size() == 0 ) return str;
		std::size_t begin = 0;
		
		while ( str[begin] == ' ' || str[begin] == '\r' 
				|| str[begin] == '\t' || str[begin] == '\n' )
			{
				++begin;
				if ( begin == str.size() ) break;
			}
		return str.substr( begin );
	}

	std::string trimRight( const std::string& str ) const
	{
		if ( str.size() == 0 ) return str;
		std::size_t end = str.size() - 1;
		
		while ( str[end] == ' ' || str[end] == '\r' 
				|| str[end] == '\t' || str[end] == '\n' )
			{
				--end;
				if ( end < 0 ) break;
			}
		return str.substr( 0, end + 1 );
	}
	
	std::string trim( const std::string& str ) const
	{
		return trimRight( trimLeft( str ) );		
	}

    virtual void readBlock()	// throw std::string
    {
        std::string tmpStr;
		std::getline( this->_file, tmpStr );
		
		while ( trim( tmpStr ) == "" )
		{
			if ( _file.eof() ) break;
			std::getline( this->_file, tmpStr );
		}
		
        while ( tmpStr != "" )
        {
			_block.push_back( tmpStr );
			if ( _file.eof() ) break;
			
			std::cout << tmpStr << std::endl;
            std::getline( this->_file, tmpStr );
			
			tmpStr = trim( tmpStr );	
        }
		
		if ( _block.size() < 3 )
			throw std::string( "wrong srt format" );
    }

    virtual void writeBlock( std::ofstream& out )
    {
        timeShift();
		for ( std::size_t i = 0; i < _block.size(); ++i )
		{
			out << _block[i] << std::endl;
		}
		out << std::endl;
		
		_block.clear();
    }

public:
	subSrt()			{}
    virtual ~subSrt()	{}
	
    virtual void doShift( std::string pathOut, int sec )
    {
        _sec = sec;
		std::ofstream myOut;
		myOut.open( pathOut.c_str() );

        while ( this->_file.good() )
        {
            readBlock();
			std::cout << "read\n";
            writeBlock( myOut );
			std::cout << "write\n";
        }
		
		myOut.close();
    }
};

//*** main *********************************************************************

void printHelp()
{

}

int main( int argc, char** argv )
{
    // parse args:

    if ( argc < 2 )
    {
        printHelp();
        return 0;
    }

    int timeShift = 0;
    std::queue< std::string > tasks;

    std::size_t i = 0;
    while ( i < argc )
    {
        if ( *(argv[i]) == '-' )
        {
            if ( std::string( argv[i] ) == std::string( "--help" ) )
            {
                printHelp();
                return 0;
            }
            if ( std::string( argv[i] ) == std::string( "-h" ) )
            {
                printHelp();
                return 0;
            }

            if ( std::string( argv[i] ) == std::string( "--shift" ) )
            {
                ++i;
                timeShift = atoi( argv[i] );
            }
            if ( std::string( argv[i] ) == std::string( "-s" ) )
            {
                ++i;
                timeShift = atoi( argv[i] );
            }
        }
        else
        {
            tasks.push( std::string( argv[i] ) );
        }
        ++i;
    }

    if ( timeShift == 0 )
    {
        std::cout << "WARNING: time shift is 0. Try to check your arguments."
                     << std::endl;
        return -1;
    }

    // do shift:
	
	std::cout << " sec: " << timeShift << std::endl;

	std::string task = std::string( "./" ) + tasks.back();
	if ( task.substr( task.size() - 4 ) != ".srt" )
	{
		std::cout << "ERROR: unknown format" << std::endl;
		return 0;
	}
	
	try
	{
		subSrt srtProcessor;
		srtProcessor.init( task );
		std::string result = task.substr( 0, task.size() - 4 ) + ".new.srt";
		srtProcessor.doShift( result, timeShift );
	}
	catch( std::string& e )
	{
		std::cout << e << std::endl;
	}
	
    return 0;
}
