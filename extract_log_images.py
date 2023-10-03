"""
when having DUMP_IMAGE enabled, the seen image is dumpend in Netpbm PGM format. This little script reads the images out of the log file

usage: python extract_log_images.py path_prefix log_file(s)
reads images out of log fileand store them as path_prefix/<nr>.pgm

"""

import sys
import os

for log_file in sys.argv[2:]:
    with open(log_file) as fin:
        fout = None
        files_written = 0
        output_file_name = ""
        line_cnt = 0
        while True:
            line = fin.readline()
            if not line:
                line_cnt = 0
                print(files_written)
                if fout:  # delete the file again
                    fout.close()
                    os.remove(output_file_name)
                    fout = None
                break
            line_cnt += 1
            if line.strip() == "P2":
                line_cnt = 0
                print(files_written)
                if fout:
                    fout.close()
                    files_written += 1
                output_file_name = os.path.join(
                    sys.argv[1], str(files_written) + ".pgm"
                )
                fout = open(output_file_name, "w")
            if fout:
                if line[:2] == "--":  # it's another debug output
                    print(line)
                else:
                    fout.write(line)
