function [result] = radial_dc( mri_data, params )
	cols = size( mri_data, 1 );
	rows = size( mri_data, 2 );

	center_col = floor( cols / 2 ) + 1;

	ramp_filt = (1:cols)' * ones(1,rows);
	ramp_filt = abs( ramp_filt - center_col );

	data_elems = numel( mri_data );
	image_size = cols*rows;
	chunks = data_elems / image_size;

	cos_filt = (1:cols)' -1;
	cos_filt = cos_filt * pi / max( cos_filt ); 
	cos_filt = cos_filt - (pi/2);
	cos_filt = cos(cos_filt) * ones(1,rows);

	for( chunk = 1:chunks )
		mri_data(:,:,chunk) = mri_data(:,:,chunk) .* ramp_filt .* cos_filt;
	end

	result = mri_data;
