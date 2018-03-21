#!/usr/bin/env python

import argparse
import logging
import shutil
import struct
import yaml

log = None

def fsize(f):
    old = f.tell()
    f.seek(0, 2)
    end = f.tell()
    f.seek(old, 0)
    return end

class BinHeader(object):
    def __init__(self, raw, last):
        self.last = last

        self.f = None
        if "file" in raw:
            self.f = raw["file"]

        self.args = []
        if "args" in raw:
            self.args = [int(a) for a in raw["args"]]

    def deps(self):
        if self.f:
            return [self.f]
        else:
            return []

    def gen(self):
        sz = 0xc

        if self.f:
            f = open(self.f)
            fsz = fsize(f)
            padsz = (4 - (fsz & 3)) & 3

            sz += fsz + padsz

        sz += len(self.args) * 4

        out = struct.pack("<BBHBxxx", 0x2, sz >> 16, sz & 0xffff, len(self.args))
        for arg in self.args:
            out += struct.pack("<L", arg)

        if self.f:
            out += f.read()
            out += "\x00" * padsz
            f.close()

        out += struct.pack("<Bxxx", 0 if self.last else 1)
        return out

class RegHeader(object):
    def __init__(self, raw, last):
        self.last = last

        self.delay = 1
        if "delay" in raw:
            self.delay = int(raw["delay"])

        self.regs = []
        for reg, val in raw["regs"]:
            self.regs.append((int(reg), int(val)))

    def deps(self):
        return []

    def gen(self):
        sz  = 0x8
        sz += len(self.regs) * 8

        out = struct.pack("<BBH", 0x3, sz >> 16, sz & 0xffff)

        for reg, val in self.regs:
            out += struct.pack("<LL", reg, val)

        out += struct.pack("<BBxx", 0 if self.last else 1, self.delay)
        return out


class Header(object):
    BOOT_SOURCE = {
        "spi":   0x5a,
        "uart0": 0x69,
        "nand":  0x8b,
    }

    SUB_HEADER = {
        "bin": BinHeader,
        "reg": RegHeader,
    }

    def __init__(self, boot, raw):
        if not boot:
            if "boot" in raw:
                boot = raw["boot"]

        if not boot or boot not in self.BOOT_SOURCE:
            raise ValueError("missing or unknown \"boot\" attribute, must be " +
                             "one of " + ", ".join(self.BOOT_SOURCE.keys()))

        self.boot = boot

        self.version = 1
        if "version" in raw:
            self.version = int(raw["version"])

        self.f = None
        if "file" in raw:
            self.f = raw["file"]

        self.align = 1
        if "file-align" in raw and raw["file-align"] != None:
            self.align = int(raw["file-align"])

        self.src, self.load, self.entry, self.bootimage = None, None, None, None
        if "src-address" in raw and raw["src-address"] != None:
            self.src = int(raw["src-address"])
        if "load-address" in raw and raw["load-address"] != None:
            self.load = int(raw["load-address"])
        if "entry-point" in raw and raw["entry-point"] != None:
            self.entry = int(raw["entry-point"])
        if "file" in raw and raw["file"] != None:
            self.bootimage = (raw["file"])

        self.headers = []
        if "headers" in raw:
            for i, hdr in enumerate(raw["headers"]):
                if "type" not in hdr or hdr["type"] not in self.SUB_HEADER:
                    raise ValueError("missing or unknown \"type\" attribute " +
                                     "in header " + str(i) + ", must be one of " +
                                     ", ".join(self.BOOT_SOURCE.keys()))

                last = True if i == len(raw["headers"]) - 1 else False

                self.headers.append(self.SUB_HEADER[hdr["type"]](hdr, last))

    def deps(self):
        deps = reduce(lambda ds, hdr: ds + hdr.deps(), self.headers, [])

        if self.f:
            deps.append(self.f)

        return deps

    def gen(self):
        sz = 0x20
        
        fsz = 0
        if self.f:
            f = open(self.f, "r")
            fsz  = fsize(f)
            fsz += (4 - (fsz & 3)) & 3
            f.close()

        subhdrs = reduce(lambda out, hdr: out + hdr.gen(), self.headers, "")

        sz += len(subhdrs)
        padsz = (self.align - (sz & (self.align - 1))) & (self.align - 1)
        sz += padsz

        out = struct.pack("<BxxxLBBHLLLxBBxxxB", self.BOOT_SOURCE[self.boot],
                          fsz, self.version, sz >> 16, sz & 0xffff,
                          self.src   if self.src   != None  else sz,
                          self.load  if self.load  != None else 0xffffffff,
                          self.entry if self.entry != None else 0,
                          0, 0, 1 if self.headers else 0)

        csum = reduce(lambda csum, b: csum + ord(b), out + subhdrs, 0)

        out += struct.pack("<B", csum & 0xff)
        
        out += subhdrs + ("\x00" * padsz)
        
        out += self.bootimage_add()
        
        return out

    def payload_csum(self):
        if not self.f:
            return 0

        f = open(self.f, "r")
        def get_block():
            return f.read(1024)

        csum = 0
        for block in iter(get_block, ""):
            csum += reduce(lambda csum, b: csum + ord(b), block, 0)

        f.close()
        return csum & 0xffffffff

    def bootimage_add(self):
        if self.bootimage == None:
            return ""
        
        f = open(self.bootimage, "r")
        
        if self.f:
            out = f.read()
            f.close()

        return out
            
    def write(self, f):
        f.write(self.gen())

        out = struct.pack("<L", self.payload_csum())
        f.write(out)

if __name__ == "__main__":
    LOG_FMT = "%(levelname)-8s %(name)8s: %(message)s"
    logging.basicConfig(format=LOG_FMT, level="INFO")
    log = logging.getLogger("mvhdr")

    parser = argparse.ArgumentParser(description="Marvell BootROM Header Compiler")

    parser.add_argument("-B", "--boot", metavar="spi|uart0|nand",
                        help="Override any boot source specified in the source file",
                        dest="boot")

    parser.add_argument("-D", "--dependencies", action="store_true", dest="deps",
                        help="Do not compile the header, instead print its " +
                        "dependencies to stdout and then exit.")

    parser.add_argument("-f", "--file", metavar="FILE", default="MVHeaderfile",
                        help="Source YAML file to compile binary header from. " +
                        "If not supplied \"MVHeaderfile\" is used.", dest="infile")

    parser.add_argument("-o", "--output", metavar="FILE", default="mvhdr.bin",
                        help="Compiled binary header destination file. " +
                        "If not supplied \"mvhdr.bin\" is used.", dest="outfile")

    args = parser.parse_args()
    raw = yaml.load(open(args.infile).read())

    hdr = Header(args.boot, raw)

    if args.deps:
        print(" ".join(hdr.deps()))
    else:
        f = open(args.outfile, "w")
        hdr.write(f)
        f.close()
