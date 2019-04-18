cmake_minimum_required (VERSION 3.1)
set (CMAKE_CXX_STANDARD 14)


set (TESTDRIVER ${CMAKE_CURRENT_SOURCE_DIR}/driver.py)
set (SIMPLIFYDRIVER ${CMAKE_CURRENT_SOURCE_DIR}/simplifydriver.py)


function(add_model_test_satis model)
	add_test (NAME ${model}_satis COMMAND /usr/bin/python ${TESTDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 0)
	add_test (NAME ${model}_simplify_satis COMMAND /usr/bin/python ${SIMPLIFYDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 0)
endfunction()

function(add_model_test_nsatis model)
	add_test (NAME ${model}_nsatis COMMAND /usr/bin/python ${TESTDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 1)
	add_test (NAME ${model}_simplify_nsatis COMMAND /usr/bin/python ${SIMPLIFYDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 1)
endfunction()

function(add_model_test_nodiea model)
	add_test (NAME ${model}_noidea COMMAND /usr/bin/python ${TESTDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 20)
	add_test (NAME ${model}_simplify_noidea COMMAND /usr/bin/python ${SIMPLIFYDRIVER} ${PROJECT_BINARY_DIR}/${ToolName} ${model} 20)
endfunction()


add_subdirectory (track1)
