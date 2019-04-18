 file(GLOB files "*.eq")
foreach(file ${files})
	 add_model_test_satis (${file})
endforeach()

