import sys

with open(sys.argv[1]) as f:
    with open(sys.argv[2], "w") as of:
        of.write("/* ****Autogenerated file! Do not edit!**** */ \\\n")
        for l in f:
            of.write(l.rstrip() + " \\\n")

        of.write("\n")
