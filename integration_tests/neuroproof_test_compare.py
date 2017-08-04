import sys
import platform
import subprocess
import os

prefix_path = sys.argv[1]
cmakepath = sys.argv[2]


if platform.system() == "Darwin":
    goldenprefix = cmakepath + "/integration_tests/outputs/mac/"
else:
    goldenprefix = cmakepath + "/integration_tests/outputs/linux/"

testoutprefix = cmakepath + "/integration_tests/temp_data/"


def compare_outputs(exe_string, stdoutfile, file_comps=None):
    exe_string = exe_string.replace("${INSTALL_PREFIX_PATH}", prefix_path)
    exe_string = exe_string.replace("${CMAKE_SOURCE_DIR}", cmakepath)

    if not os.path.exists(testoutprefix):
        os.makedirs(testoutprefix)

    golden_stdout_name = goldenprefix + stdoutfile
    testout_stdout_name = testoutprefix + stdoutfile

    p = subprocess.Popen(exe_string.split(), stdout=subprocess.PIPE)
 
    testout_stdout, err = p.communicate()
    testout_stdout = testout_stdout.decode('utf-8')

    # error returned from call
    if err is not None:
        sys.stderr.write("Error communicating with subprocess\n")
        exit(1)

    testout_stdout_arr = testout_stdout.split('\n')
    golden_stdout = open(golden_stdout_name).read()
    golden_stdout = golden_stdout.replace("-nan", "nan")

    testout_stdout = ""
    for line_index in range(0, len(testout_stdout_arr)):
        line = testout_stdout_arr[line_index]
        line = line.replace("-nan", "nan")
        if "Time " in line:
            continue
        testout_stdout += line
        if line_index != (len(testout_stdout_arr)-1):
            testout_stdout += "\n"

    fout = open(testout_stdout_name, 'w')
    fout.write(testout_stdout)  
    fout.close()

    if testout_stdout != golden_stdout:
        sys.stderr.write("testout != golden_stdout\n")
        exit(1)

    if file_comps is not None:
        for file_comp in file_comps:
            fing = open(goldenprefix + file_comp)
            fint = open(testoutprefix + file_comp)

            g_out = fing.read()
            t_out = fint.read()

            g_out = g_out.replace("-nan", "nan")
            t_out = t_out.replace("-nan", "nan")

            if g_out != t_out:
                sys.stdout.write("g_out != t_out\n")
                exit(1)

            fing.close()
            fint.close()

    print("SUCCESS")



