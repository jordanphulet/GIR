#include <GIRServer.h>
#include <GIRLogger.h>
#include <GIRUtils.h>
#include <MRIDataComm.h>
#include <ReconPipeline.h>
#include <GIRXML.h>
#include <stdio.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

#define GIR_PORT 9999
#define GIR_LOG_PATH "/etc/gir/GIR.log"
#define GIR_PLUGIN_DIR "/etc/gir/plugins/"
#define GIR_PIPELINE_DIR "/etc/gir/pipelines/"
#define GIR_PMU_DIR "/etc/gir/pmu/"
#define GIR_CINE_TSE_BINS 8

GIRServer::GIRServer():
	port( GIR_PORT ),
	plugin_dir( GIR_PLUGIN_DIR ),
	pipeline_dir( GIR_PIPELINE_DIR ),
	pmu_dir( GIR_PMU_DIR )
{
}

bool GIRServer::Configure( GIRConfig& new_config, bool main_config, bool final_config )
{
	if( main_config )
	{
		// get parameters
		new_config.GetParam( "", "", "port", port );
		new_config.GetParam( "", "", "plugin_dir", plugin_dir );
		new_config.GetParam( "", "", "pipeline_dir", pipeline_dir );
		new_config.GetParam( "", "", "log_path", log_path );
		GIRUtils::CompleteDirPath( plugin_dir );
		GIRUtils::CompleteDirPath( pipeline_dir );
	
		// check for plugins_dir
		if( !GIRUtils::IsDir( plugin_dir.c_str() ) )
		{
			GIRLogger::LogError( "GIRServer::CheckParameters -> plugins_dir \"%s\" is invalid!\n", plugin_dir.c_str() );
			return false;
		}
	
		// check for pipeline_dir
		if( !GIRUtils::IsDir( pipeline_dir.c_str() ) )
		{
			GIRLogger::LogError( "GIRServer::CheckParameters -> pipelines_dir \"%s\" is invalid!\n", pipeline_dir.c_str() );
			return false;
		}
	}

	return true;
}

bool GIRServer::AcceptConnection()
{
	return communicator.AcceptConnection();
}

void GIRServer::CloseConnection()
{
	communicator.CloseConnection();
}

bool GIRServer::StartListening()
{
	return communicator.Listen( port );
}

void GIRServer::StopListening()
{
	communicator.StopListening();
}

void GIRServer::ProcessRequest( GIRConfig& main_config )
{
	// purge old data from communicator
	communicator.Purge();

	// attempt to reconstruct
	MRIData data;
	MRIReconRequest request;
	MRIReconAck ack;
	TryReconstruct( data, request, ack, main_config );

	// send ack and data if request wasn't silent
	if( !request.silent )
	{
		// send ack, and data if ack.success
		GIRLogger::LogInfo( "sending ACK...\n" );
		if( !communicator.SendReconAck( ack ) )
			GIRLogger::LogError( "GIRServer::ProcessRequest -> SendReconAck failed!\n" );
		else if( ack.success )
		{
			GIRLogger::LogInfo( "sending data...\n" );
			if( !communicator.SendData( data ) )
				GIRLogger::LogError( "GIRServer::ProcessRequest -> SendData failed!\n" );
	
			// wait for ack to close connection
			GIRLogger::LogInfo( "waiting for response ack...\n" );
			MRIReconAck re_ack;
			if( !communicator.ReceiveReconAck( re_ack ) )
				GIRLogger::LogError( "GIRServer::ProcessRequest -> ReceiveReconAck failed!\n" );
		}
	}
	else
		GIRLogger::LogInfo( "request was silent so no data will be sent back...\n" );
}

std::string GIRServer::GetConfigString() const
{
	std::stringstream stream;
	stream.width( 20 ); stream << right << "port: " << port << std::endl;
	stream.width( 20 ); stream << right << "log_path: " << log_path << std::endl;
	stream.width( 20 ); stream << right << "plugin_dir: " << plugin_dir << std::endl;
	stream.width( 20 ); stream << right << "pipeline_dir: " << pipeline_dir << std::endl;
	return stream.str();
}

void GIRServer::TryReconstruct( MRIData& data, MRIReconRequest& request, MRIReconAck& ack, GIRConfig& main_config )
{
	ack.success = false;

	// get request
	if( !communicator.ReceiveReconRequest( request ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> couldn't receive request!\n" );
		ack.message = "ReceiveRequest failed!";
		return;
	}

	// get header
	GIRLogger::LogInfo( "waiting for header...\n" );
	MRIDataHeader header;
	if( !communicator.ReceiveDataHeader( header ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> couldn't receive header!\n" );
		ack.message = "ReceiveDataHeader failed!";
		return;
	}
	GIRLogger::LogInfo( "header received: %s\n", header.Size().ToString().c_str() );

	// get data
	GIRLogger::LogInfo( "waiting for data...\n" );
	MRIMeasurement meas;
	std::vector<MRIMeasurement> meas_vector;
	while( communicator.ReceiveMeasurement( meas ) )
		meas_vector.push_back( meas );
	if( communicator.BufferDataType() != SER_END_SIGNAL )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> never received end signal!\n" );
		ack.message = "Never recieved END_SIGNAL!";
		return;
	}
	GIRLogger::LogInfo( "data received\n" );

	// load pipeline
	std::string pipeline_path = pipeline_dir + request.pipeline + ".xml";
	GIRLogger::LogInfo( "loading pipeline %s...\n", pipeline_path.c_str() );
	ReconPipeline pipeline;
	if( !GIRXML::Load( pipeline_path.c_str(), pipeline, plugin_dir.c_str() ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> unable to load requested pipeline: \"%s!\"\n", pipeline_path.c_str() );
		ack.message = "Failed to load pipeline!";
		return;
	}
	
	// load pipeline config
	GIRConfig pipeline_config;
	if( !GIRXML::Load( pipeline_path.c_str(), pipeline_config ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> unable to load config from requested pipeline: \"%s\"\n", pipeline_path.c_str() );
		ack.message = "Failed to load pipeline config!";
		return;
	}

	// configure pipeline
	if( !pipeline.Configure( main_config, true, false ) || !pipeline.Configure( pipeline_config, false, false ) || !pipeline.Configure( request.config, false, true ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> configure pipeline failed for pipeline: \"%s!\"\n", pipeline_path.c_str() );
		ack.message = "Failed to configure pipeline!";
		return;
	}

	// initialize mri data with information from header
	data = MRIData( header.Size(), header.IsComplex() );
	// reconstruct
	GIRLogger::LogInfo( "reconstructing...\n" );
	if( !pipeline.Reconstruct( meas_vector, data ) )
	{
		GIRLogger::LogError( "GIRServer::TryReconstruct -> reconstruction failed for pipeline: %s!\n", pipeline_path.c_str() );
		ack.message = "Reconstruction failed!";
		return;
	}

	ack.success = true;
}

void GIRServer::PrintGIR() const
{
	std::stringstream stream;
	stream <<  "\n\n";
	stream <<  "                         ._<a'                :w,. \n";
	stream <<  "                        wmQW(                  4Qma\n";
	stream <<  "                       <WQ@'                   ]QQQ\n";
	stream <<  "                       jQ@'                    ]QQW\n";
	stream <<  "                       mQ(                     =QQE\n";
	stream <<  "                      .QP                      =QQ[\n";
	stream <<  "                      .Q[_____________.        ]QW`\n";
	stream <<  "               ._aawdYY?9!!!!!!:!!!??TYSXmaa,  jQf \n";
	stream <<  "             apY?!~..::.'::...'':.:..::.:.'*9q,Q@` \n";
	stream <<  "           .jP::.'.'..&'..................':::$Q(  \n";
	stream <<  "          .yC:..........:................::::::Wk  \n";
	stream <<  "          y[::.....:)'.................:':::::vQ'  \n";
	stream <<  "         jf:......:<..................:'::::::mF   \n";
	stream <<  "        <P:........+........:::::::....':::::jW`   \n";
	stream <<  "       _Qas,.:....<.....::'awwwgwwa,..::::::<Q[    \n";
	stream <<  "   .s&?!!!?9qa...::....'wmT!^- --!?$gc':::::j@     \n";
	stream <<  "  _e`       -?m.'.>:..<mP`         -?Qp:::::Q'     \n";
	stream <<  " _2          .]m.'(:.:mP            .+Wz:::qF      \n";
	stream <<  " j`          .:Q>....]W'             .]Q:::Q'      \n";
	stream <<  " k  ).       .'Q)'c:.jD          v. .-]W::3E       \n";
	stream <<  " 4.          :j@:'+..]Q.             :dE::m(       \n";
	stream <<  " -5,       ..j@~.'s:.'$L            :jW::v@        \n";
	stream <<  "   ?a, ..._amT:..:::..+$w.        ._y@(::jf        \n";
	stream <<  "     !!*nmV!+:..'noah:.:Y$wc,.__'ayWY<::<Q'        \n";
	stream <<  "         4w...:.:.'1':..::!?TVVVV?!'.'::]E         \n";
	stream <<  "          ?ma::::::l:':':':'''.::.'...::d[         \n";
	stream <<  "           -VUVVTTWUWUW???????Xpnaa...::Q          \n";
	stream <<  "                 )ElSl3c    w2n::u9(:::<W          \n";
	stream <<  "                 UXS2SYC    -{Zh3>&:jmanF          \n";
	stream <<  "                             d+2c<(:mQQmf          \n";
	stream <<  "                            ]C<()v::$QQQ[          \n";
	stream <<  "                           _#'&{&(:.]QQ$[          \n";
	stream <<  "                           jC:C1I.:.'VYd[ _>       \n";
	stream <<  "                           ]g'.'''''''+dmZ^.       \n";
	stream <<  "                            !Qmggnnowwwm(          \n";
	stream <<  "                             )WWQ:  ]QWQ`          \n";
	stream <<  "                              !H#'  -WQQ`          \n\n";
	stream <<  "            GIR REPORTING FOR DUTY!                \n\n";    
	GIRLogger::LogInfo( stream.str().c_str() );
 }
