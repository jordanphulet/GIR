#ifndef __GIR_H__
#define __GIR_H__

#include <TCPCommunicator.h>
#include <GIRConfig.h>
#include <GIRUtils.h>

class ReconPipeline;

class GIRServer: public GIRConfigurable
{
	public:
	GIRServer();

	bool Configure( GIRConfig& new_config, bool main_config, bool final_config );
	bool AcceptConnection();
	void CloseConnection();
	bool StartListening();
	void StopListening();
	void ProcessRequest( GIRConfig& main_config );

	const std::string LogPath() const { return log_path; }
	std::string GetConfigString() const;
	void PrintGIR() const;

	private:
	TCPCommunicator communicator;
	int port;
	std::string log_path;
	std::string plugin_dir;
	std::string pipeline_dir;
	std::string pmu_dir;

	void TryReconstruct( MRIData& data, MRIReconRequest& request, MRIReconAck& ack, GIRConfig& main_config );
};

#endif
