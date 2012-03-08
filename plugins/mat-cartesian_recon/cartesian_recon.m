function [result] = cartesian_recon( mri_data, params )
	result = ifft( ifft( mri_data, [], 1), [], 2 );
	result = fftshift( fftshift( result, 1 ), 2 );
	result = sqrt( sum( abs( result ).^2, 3 ) );
