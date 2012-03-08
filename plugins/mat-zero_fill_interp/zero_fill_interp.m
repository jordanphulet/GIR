function [result] = zero_fill_interp( mri_data, params )
	result = mri_data;

	result = fft( fft( mri_data, [], 1 ), [], 2 );
	result = fftshift( fftshift( result, 1 ), 2 );

	% x zero fill
	if( isfield( params, 'x_factor' ) )
		x_factor = str2num( params.x_factor );
		margin_size = floor( size(result,2)*x_factor / 4 );

		zeros_size = size( result );
		zeros_size(2) = margin_size;

		result = cat( 2, ...
			zeros( zeros_size ), ...
			result, ...
			zeros( zeros_size ) );
	end

	% y zero fill
	if( isfield( params, 'y_factor' ) )
		y_factor = str2num( params.y_factor );
		margin_size = floor( size(result,1)*y_factor / 4 );

		zeros_size = size( result );
		zeros_size(1) = margin_size;

		result = cat( 1, ...
			zeros( zeros_size ), ...
			result, ...
			zeros( zeros_size ) );
	end

	result = ifftshift( ifftshift( result, 1 ), 2 );
	result = ifft( ifft( result, [], 1 ), [], 2 );
