import gdb
import re
import struct
import sys

def inf():
    return gdb.selected_inferior()

def hd(base, mem):
    i = 0
    while len(mem) >= 4:
        if i % 4 == 0:
            sys.stdout.write("%8.8x:" % base)

        sys.stdout.write(" %8.8x" % struct.unpack("<L", mem[:4]))

        if i % 4 == 3:
            sys.stdout.write("\n")

        i += 1
        base += 4
        mem = mem[4:]

def bold(text):
    return "\x1b[1m" + text + "\x1b[0m"

def flagstr(flags):
    def _flagstr_one(f):
        v = flags[f]()
        if v:
            return bold("+" + f)
        else:
            return "-" + f

    fs = sorted([f for f in flags])
    return ",".join([_flagstr_one(f) for f in fs])

def bit(n, val):
    return bool(val & (1 << n))


class XC3(object):
    def __init__(self, addr, win_no=6):
        self.base, self.win_no = addr, win_no
        self.win_base = None

    def win_addr(self, addr):
        win_base = addr >> 19
        if win_base != self.win_base:
            win_reg = self.base + 0x120 + (self.win_no << 2)
            inf().write_memory(win_reg, struct.pack("<L", win_base))
            self.win_base = win_base

        return self.base + (self.win_no << 19) + (addr & 0x7ffff)

    def read(self, addr, size):
        return inf().read_memory(self.win_addr(addr), size)

    def read32(self, addr):
        return struct.unpack("<L", self.read(addr, 4))[0]

    def write(self, addr, data):
        return inf().write_memory(self.win_addr(addr), data)

    def write32(self, addr, value):
        return self.write(addr, struct.pack("<L", value))

class Queue(object):
    def __init__(self, xc3, q_no):
        self.xc3, self.q_no = xc3, q_no

        self.flags = {
            "ena": self.enabled,
            "dis": self.disabled,
            "buf": self.buffer,
            "err": self.error,
        }

    def __str__(self):
        return "{name}: flags:{fs}".format(name=self.name, fs=flagstr(self.flags))

    def enabled(self):
        qcmd = self.xc3.read32(self.addr_qcmd)
        return bit(self.q_no, qcmd)

    def disabled(self):
        qcmd = self.xc3.read32(self.addr_qcmd)
        return bit(self.q_no + 8, qcmd)

    def buffer(self):
        irq = self.xc3.read32(self.addr_irq)
        return bit(self.q_no + self.irq_buffer, irq)

    def error(self):
        irq = self.xc3.read32(self.addr_irq)
        return bit(self.q_no + self.irq_error, irq)

class RxQueue(Queue):
    def __init__(self, xc3, q_no):
        super(RxQueue, self).__init__(xc3, q_no)

        self.name = "rxq-%d" % q_no
        self.addr_qcmd = 0x2680
        self.addr_irq  = 0x280c
        self.irq_buffer = 2
        self.irq_error  = 11
        self.flags["pnd"] = self.pending

    def __str__(self):
        qstr = super(RxQueue, self).__str__()
        return qstr + " pkts:%-4d bytes:%-5d" % (self.packets(), self.bytes())

    def pending(self):
        status = self.xc3.read32(0x281c)
        return not bit(self.q_no, status)

    def packets(self):
        return self.xc3.read32(0x2820 + (self.q_no << 2))

    def bytes(self):
        return self.xc3.read32(0x2840 + (self.q_no << 2))

    def descriptor(self):
        return RxDescriptor(self.xc3, self.xc3.read32(0x260c + (self.q_no << 4)))

class TxQueue(Queue):
    def __init__(self, xc3, q_no):
        super(TxQueue, self).__init__(xc3, q_no)

        self.name = "txq-%d" % q_no
        self.addr_qcmd = 0x2868
        self.addr_irq  = 0x2810
        self.irq_buffer = 1
        self.irq_error  = 9
        self.flags["end"] = self.end

    def end(self):
        irq = self.xc3.read32(self.addr_irq)
        return bit(self.q_no + 17, irq)

    def descriptor(self):
        return TxDescriptor(self.xc3, self.xc3.read32(0x26c0 + (self.q_no << 2)))

class Descriptor(object):
    def __init__(self, xc3, addr):
        self.xc3, self.addr = xc3, addr
        self.flags = {
            "irq": self.irq,
            "fst": self.first,
            "lst": self.last,
        }

        mem = inf().read_memory(self.addr, 0x10)
        self._cmd, self._size, self._count, self._data, self._next = struct.unpack("<LHHLL", mem)

    def __str__(self):
        return "addr:%8.8x owner:%-4s flags:%s errors:%s size:%#x/%#x" % (
            self.addr, self.owner(), flagstr(self.flags), flagstr(self.errors),
            self.count(), self.size())

    def owner(self):
        return "sdma" if bit(31, self._cmd) else "cpu"

    def size(self):
        return self._size
    
    def count(self):
        return self._count & 0x2fff

    def frame(self):
        return Frame(self._data, self.count())

class RxDescriptor(Descriptor):
    def __init__(self, xc3, addr):
        super(RxDescriptor, self).__init__(xc3, addr)
        self.errors =  {
            "bus": self.bus_error,
            "res": self.resource_error,
            "crc": self.crc_error,
        }
    
    def irq(self):
        return bit(29, self._cmd)

    def first(self):
        return bit(27, self._cmd)

    def last(self):
        return bit(26, self._cmd)

    def bus_error(self):
        return bit(30, self._cmd)

    def resource_error(self):
        return bit(28, self._cmd)

    def crc_error(self):
        return bit(14, self._count)

    def next(self):
        return RxDescriptor(self.xc3, self._next)

class TxDescriptor(Descriptor):
    def __init__(self, xc3, addr):
        super(TxDescriptor, self).__init__(xc3, addr)
        self.errors =  {
            "crc": self.crc_error,
        }
    
    def irq(self):
        return bit(23, self._cmd)

    def first(self):
        return bit(21, self._cmd)

    def last(self):
        return bit(20, self._cmd)

    def crc_error(self):
        return bit(12, self._cmd)

    def next(self):
        return TxDescriptor(self.xc3, self._next)

class Frame(object):
    def __init__(self, addr, size):
        self.addr, self.size = addr, size

    def __str__(self):
        return "frame: addr:%8.8x size:%#x" % (self.addr, self.size)

class GBE(object):
    def __init__(self, xc3, port):
        self.xc3, self.port = xc3, port
        self.flags = {
            "lnk": self.link,
            "dpx": self.duplex,
            "press": self.pressure,
            "aneg": self.aneg,
            "bypass": self.aneg_bypass,
            "sync": self.sync,
        }

        self.flow =  {
            "rx": self.rx_flow,
            "tx": self.tx_flow,
        }

        self.pause =  {
            "rx": self.rx_pause,
            "tx": self.tx_pause,
        }
        self.errors =  {
            "buf": self.buffers_error,
            "sync10ms": self.sync_error,
            "pll": self.pll_error,
            "squelch": self.squelch_error,
        }
        
    def __str__(self):
        return "%-7s: speed:%-4d flags:%s flow:%s pause:%s errors:%s" % (
            self.name(), self.speed(), flagstr(self.flags),
            flagstr(self.flow), flagstr(self.pause), flagstr(self.errors))
    
    def name(self):
        if self.port == 31:
            return "gbe-cpu"
        else:
            return "gbe-%d" % self.port
        
    def mac_read(self, addr):
        return self.xc3.read32(0x12000000 + 0x1000*self.port + addr)

    def speed(self):
        r = self.mac_read(0x10)
        if bit(1, r):
            return 1000
        elif bit(2, r):
            return 100
        else:
            return 10

    def link(self):
        return bit(0, self.mac_read(0x10))
    def duplex(self):
        return bit(3, self.mac_read(0x10))
    def rx_flow(self):
        return bit(4, self.mac_read(0x10))
    def tx_flow(self):
        return bit(5, self.mac_read(0x10))
    def rx_pause(self):
        return bit(6, self.mac_read(0x10))
    def tx_pause(self):
        return bit(7, self.mac_read(0x10))
    def pressure(self):
        return bit(8, self.mac_read(0x10))
    def buffers_error(self):
        return bit(9, self.mac_read(0x10))
    def sync_error(self):
        return bit(10, self.mac_read(0x10))
    def aneg(self):
        return bit(11, self.mac_read(0x10))
    def aneg_bypass(self):
        return bit(12, self.mac_read(0x10))
    def pll_error(self):
        return not bit(13, self.mac_read(0x10))
    def sync(self):
        return bit(14, self.mac_read(0x10))
    def squelch_error(self):
        return not bit(15, self.mac_read(0x10))

xc3 = XC3(0x80000000)

rxq = [RxQueue(xc3, i) for i in range(8)]
txq = [TxQueue(xc3, i) for i in range(8)]

mac = {i: GBE(xc3, i) for i in range(24)}
mac["cpu"] = GBE(xc3, 31)


class CmdXC3 (gdb.Command):
    def __init__(self):
        super (CmdXC3, self).__init__ ("xc3", gdb.COMMAND_USER, prefix=True)

    def usage(self):
        print("XCat 3 packet processor commands\n")
CmdXC3()

class CmdRead (gdb.Command):
    def __init__(self):
        super (CmdRead, self).__init__ ("xc3 read", gdb.COMMAND_USER)

    def usage(self):
        print("Read packet processor register\n" +
              "xc3 read ADDRESS [SIZE]\n")

    def invoke (self, args, from_tty):
        parsed = [gdb.parse_and_eval(arg) for arg in args.split()]
        if len(parsed) < 1:
            return self.usage()

        addr = int(str(parsed[0]), 0)
        size = int(str(parsed[1]), 0) if len(parsed) > 1 else 0x100

        hd(addr, xc3.read(addr, size))
CmdRead()

class CmdWrite (gdb.Command):
    def __init__(self):
        super (CmdWrite, self).__init__ ("xc3 write", gdb.COMMAND_USER)

    def usage(self):
        print("Write packet processor register\n" +
              "xc3 write ADDRESS VALUE\n")

    def invoke (self, args, from_tty):
        parsed = [gdb.parse_and_eval(arg) for arg in args.split()]
        if len(parsed) != 2:
            return self.usage()

        addr = int(str(parsed[0]), 0)
        value = int(str(parsed[1]), 0)
        xc3.write32(addr, value)
CmdWrite()

class CmdQueue (gdb.Command):
    def __init__(self):
        super (CmdQueue, self).__init__ ("xc3 queue", gdb.COMMAND_USER, prefix=True)

    def usage(self):
        print("Display Rx/Tx queue status\n" +
              "xc3 queue\n")

    def invoke (self, args, from_tty):
        print("\n".join(map(str, rxq)))
        print("\n".join(map(str, txq)))
        return
CmdQueue()

class CmdRing (gdb.Command):
    def __init__(self):
        super (CmdRing, self).__init__ ("xc3 queue " + self.path, gdb.COMMAND_USER)

    def usage(self):
        print("Display %s-queue ring\n" % self.path +
              "xc3 queue %s QUEUE\n" % self.path)

    def invoke (self, args, from_tty):
        parsed = [gdb.parse_and_eval(arg) for arg in args.split()]

        if len(parsed) != 1:
            return self.usage()

        q = self.qs[int(str(parsed[0]), 0)]

        first = q.descriptor()
        ds = [first]
        for _ in range(1024):
            d = ds[-1].next()
            if d.addr == ds[0].addr:
                break

            ds.append(d)

        ds.sort(key=lambda d: d.addr)
        for d in ds:
            if d == first:
                print("-> " + str(d))
            else:
                print("   " + str(d))

class CmdRxRing(CmdRing):
    path = "rx"
    qs = rxq
CmdRxRing()

class CmdTxRing(CmdRing):
    path = "tx"
    qs = txq
CmdTxRing()

class CmdMAC(gdb.Command):
    def __init__(self):
        super (CmdMAC, self).__init__ ("xc3 mac", gdb.COMMAND_USER)

    def usage(self):
        print("Display port MAC status\n" +
              "xc3 mac [MAC]\n")

    def invoke (self, args, from_tty):
        parsed = [gdb.parse_and_eval(arg) for arg in args.split()]

        if len(parsed) == 0:
            pnames = sorted(mac.keys(), key=lambda n: n if type(n) == int else -1)
            [print(str(mac[name])) for name in pnames]
            return
        
        if len(parsed) != 1:
            return self.usage()

        name = parsed[0].string()
        if name not in mac:
            print("Unknown MAC %s\n" % name +
                  "Expected one of: " + ", ".join([n for n in mac]))
            return

        print(str(mac[name]))
CmdMAC()
