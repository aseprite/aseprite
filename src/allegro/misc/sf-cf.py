#!/usr/bin/env python
import os, sys, optparse, threading, time

# Put here any hosts you want to compile on.
hosts = [

    "x86-linux2", #: Fedora Linux FC2 running Linux 2.6 kernel
    "x86-openbsd1", #: OpenBSD 3.4
    "x86-solaris1", #: Sun Solaris 9
    "x86-linux1", #: Debian GNU/Linux 3.0 running Linux 2.4 kernel
    "x86-freebsd1", #: FreeBSD 4.8
    "x86-netbsd1", #: NetBSD 1.6.1

    "amd64-linux1", #: Fedora Core release 3 running Linux 2.6 kernel
    "amd64-linux2", #: Fedora Core release 3 running Linux 2.6 kernel

    "alpha-linux1", #: Debian GNU/Linux 3.0 running Linux 2.2 kernel

    "ppc-osx1", #: Apple Mac OS X 10.1 Server with Fink running on an Apple Mac G4
    "ppc-osx2", #: Apple Mac OS X 10.2 Server with Fink running on an Apple Mac G4
    "ppc-osx3",

    "sparc-solaris1",
    "sparc-solaris2", #: Sun Solaris 9, running on two Sun Enterprise 220R systems

    "openpower-linux1", #: SuSE Enterprise Linux 9, running on an IBM OpenPower 720 (e-Series) host.
]

# The name of a directory containing the sources to be compiled.
source_folder = "allegro"

def exe(c):
    print ">>>" + c + "\n"
    r = os.popen(c).read()
    print r

parser = optparse.OptionParser()

parser.set_usage(
"""
python %s [options]

To use this script, you must have ssh access to the SourceForge compile farm,
and your home directory must contain a directory called "allegro". The existence
of .txt files with the name of the various hosts controls which systems are
tested - by default all for which no .txt file exists yet.
""".strip() % sys.argv[0])

hostname = os.popen("hostname").read().strip()
parser.add_option("-s", "--ssh", action = "store_true", dest = "ssh",
    help = "Connect to all hosts for which no .txt files exists and " +
    "try to compile there, pasting the results back to a .txt file.")
parser.add_option("-c", "--compile", action = "store", dest = "compile",
    help = "Internal command to directly compile on the current host. Do not use.")
parser.add_option("-o", "--one", action = "store", dest = "one",
    help = "Compile only on the given host, and overwrite the .txt " +
    "file for it if it exists already.")
parser.add_option("-u", "--user",
    help = "The SourceForge compile farm username to use.")
options, args = parser.parse_args()

user = options.user

def compile(host):
    print ">>>Running on %s [%s].\n" % (host, time.asctime())
    make = "make"
    if host.find("bsd") >= 0: make = "gmake"
    name = user[:1] + "/" + user[:2] + "/" + user + "/" + host
    configure = "./configure --prefix=/home/users/%s/local" % name
    if host.find("osx") >= 0:
        configure = "./fix.sh macosx"

    commands = " ".join([
        "rm -r %s ;" % host,
        "mkdir %s ; mkdir %s/local ; " % (host, host),
        "cd %s &&" % host,
        #"gunzip -c ../allegro-4.2.1.tar.gz | tar xf - && cd allegro-4.2.1 && ",
        "cp -r ../%s . && cd %s && " % (source_folder, source_folder),
        "%s && " % configure,
        "%s &&" % make,
        "%s install" % make])
    connect_command = r'''ssh %s  "'%s'"''' % (host, commands)
    sshcommand = "ssh cf-shell.sf.net %s" % connect_command
    print sshcommand
    stdin, stdout = os.popen4(sshcommand, bufsize=1)
    print stdout.read()

def run(host):
    print "Connecting to %s..\n" % host
    print os.popen(
        "python -u compile.py --compile %s 2>&1 | tee %s.txt" %
         (host, host)).read()

if options.ssh:
    threads = []
    for host in hosts:
        # Delete previous result .txt to try again!
        if not os.path.exists("%s.txt" % host):
            threads.append(threading.Thread(None, run, None, (host, )))
    for t in threads: t.start()
    for t in threads: t.join()

elif options.compile:
    compile(options.compile)
elif options.one:
    run(options.one)
else:
    print "Need either --ssh or --compile argument. -h or --help for help."
    print
    parser.print_help()
