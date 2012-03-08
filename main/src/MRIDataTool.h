#include "FilterTool.h"
#include "MRIData.h"
#include <math.h>

#ifndef __MRI_DATA_TOOL_H_
#define __MRI_DATA_TOOL_H_

class MRIDataTool {
	public:
		static bool GetCoilSense( const MRIData &k_space, MRIData& coil_map );
		static void GetCoilMap( const MRIData &k_space, MRIData& coil_map );
		static void GetSampleMask( const MRIData &k_space, MRIData& sample_mask );
		static void GetSSQImage( const MRIData &k_space, MRIData& image_estimate );


		// phase-contrast
		static bool GetSSQPhaseContrastEstimate( MRIData &k_space, MRIData& image_estimate );
		static void GetPhaseContrastLambdaMap( const MRIData &image_estimate, MRIData& lamda_map );

		//static void DensityCompensateRadial( MRIData &k_space, float factor );

		//static void ScaleKSpaceForRecon( MRIData *k_space );

		static bool TemporallyInterpolateKSpace( const MRIData &k_space, MRIData& interp_k );
};

#endif
