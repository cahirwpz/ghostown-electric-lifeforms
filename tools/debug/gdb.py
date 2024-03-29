#!/usr/bin/env python3

import asyncio
import logging


memory_map_xml = """<?xml version="1.0"?>
<!DOCTYPE memory-map PUBLIC "+//IDN gnu.org//DTD GDB Memory Map V1.0//EN"
 "http://sourceware.org/gdb/gdb-memory-map.dtd">
<memory-map>
{}
</memory-map>"""


class GdbConnection():
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer

    @staticmethod
    def chksum(packet):
        return sum(ord(c) for c in packet) & 255

    async def recv_ack(self):
        # read a character
        data = await self.reader.read(1)
        logging.debug("(gdb) <- {}".format(data))

        # fail if not acknowledge and not retransmission marker
        if data not in [b'+', b'-']:
            raise ValueError(data)

        return data == b'+'

    async def recv_break(self):
        data = await self.reader.read(1)
        logging.debug("(gdb) <- {}".format(data))
        assert data == b'\x03'

    async def recv_packet(self):
        packet = None

        while not packet or self.reader._buffer:
            # read first character
            data = await self.reader.read(1)

            # end of stream ?
            if not data:
                return ''

            # packet start marker ?
            if data != b'$':
                raise ValueError(data)

            # read packet and decode it
            raw_packet = await self.reader.readuntil(b'#')
            packet = raw_packet.decode()[:-1]
            raw_chksum = await self.reader.read(2)
            chksum = int(raw_chksum.decode(), 16)
            logging.debug("(gdb) <- '{}'".format(packet))

            # check if check sum matches
            if chksum != self.chksum(packet):
                raise ValueError('Packet checksum is invalid!')

        return packet

    def send_nack(self, packet=None):
        logging.debug("(gdb) -> '-'")
        self.writer.write(b'-')
        if packet is not None:
            self.send(packet)

    def send_ack(self, packet=None):
        logging.debug("(gdb) -> '+'")
        self.writer.write(b'+')
        if packet is not None:
            self.send(packet)

    def send(self, packet):
        data = '${}#{:02x}'.format(packet, self.chksum(packet))
        logging.debug("(gdb) -> '{}'".format(packet))
        self.writer.write(data.encode())


class GdbStub():
    __regs__ = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7',
                'A0', 'A1', 'A2', 'A3', 'A4', 'A5', 'A6', 'A7',
                'SR', 'PC']

    def __init__(self, gdb, uae):
        self.gdb = gdb
        self.uae = uae
        self.brks = set()

    async def handle_query(self, packet):
        if packet == 'C':
            # Return the current thread ID.
            self.gdb.send_ack('QC0')
        elif packet == 'Offsets':
            # Get section offsets that the target used when relocating
            # the downloaded image.
            segs = await self.uae.get_segments()
            if segs:
                self.gdb.send_ack('Text={:08x};Data={:08x};Bss={:08x}'
                                  .format(segs[0], segs[1], segs[2]))
            else:
                self.gdb.send_ack('')
        elif packet.startswith('Supported'):
            supported = ['PacketSize=4096', 'qXfer:memory-map:read+',
                         'hwbreak+']
            self.gdb.send_ack(';'.join(supported))
        elif packet.startswith('Xfer:memory-map:read:'):
            memmap = await self.uae.memory_map()
            entries = []
            for start, length, desc in sorted(memmap):
                entries.append('<memory type="{}" start="{}" length="{}"/>'
                               .format(desc, hex(start), hex(length)))
            layout = '\n'.join(entries)
            self.gdb.send_ack('l' + memory_map_xml.format(layout))
        elif packet == 'TStatus':
            # Is there a trace experiment running right now?
            self.gdb.send_ack('T0')
        elif packet == 'TfV' or packet == 'TfP':
            self.gdb.send_ack('')
        elif packet == 'fThreadInfo':
            # Obtain a list of all active thread IDs from the target
            # (OS). The first query of the sequence will be the
            # 'qfThreadInfo' query; subsequent queries in the sequence
            # will be the 'qsThreadInfo' query.
            self.gdb.send_ack('m0')
        elif packet == 'sThreadInfo':
            # lower case letter 'L' denotes end of list
            self.gdb.send_ack('l')
        elif packet == 'Attached':
            # Return an indication of whether the remote server
            # attached to an existing process or created a new process.
            # ‘1’: The remote server attached to an existing process.
            self.gdb.send_ack('1')
        elif packet == 'Symbol::':
            # Notify the target that GDB is prepared to serve symbol
            # lookup requests.
            self.gdb.send_ack('OK')
        elif packet.startswith('Rcmd,'):
            # Forwards input from `monitor` command to UAE debugger.
            raw_cmd = packet.split(',', maxsplit=1)[1]
            cmd = bytes.fromhex(raw_cmd).decode('utf-8')
            lines = await self.uae.communicate(cmd)
            text = '\n'.join(lines) + '\n'
            self.gdb.send_ack(text.encode('utf-8').hex())

        return True

    def handle_file(self, packet):
        # Minimal support for `remote get` command to transfer empty files.
        cmd, args = packet[0], packet[1].split(',')
        if cmd == 'setfs':
            self.gdb.send_ack('F0')
        elif cmd == 'open':
            self.gdb.send_ack('F0')
        elif cmd == 'close':
            self.gdb.send_ack('F0')
        elif cmd == 'pread':
            self.gdb.send_ack('F0;')  # `pread` command requires semicolon
        else:
            self.gdb.send_ack('')

    @staticmethod
    def binary_decode(data):
        # Decode GDB binary stream. See:
        # https://sourceware.org/gdb/onlinedocs/gdb/Overview.html#Binary-Data
        escape = False
        result = bytearray()
        for x in data.encode():
            if escape:
                result.append(x ^ 0x20)
                escape = False
            elif x == 0x7d:  # '}' escape
                escape = True
            else:
                result.append(x)
        return result

    async def handle_packet(self, packet):
        if packet == '?':
            # Indicate the reason the target halted.
            self.gdb.send_ack('S05')
        if packet == '!':
            self.gdb.send_ack('OK')
        elif packet[0] == 'q':
            # Packets starting with ‘q’ are general query packets.
            return await self.handle_query(packet[1:])
        elif packet[0] == 'D':
            self.gdb.send_ack('OK')
            self.uae.resume()
            raise asyncio.CancelledError
        elif packet[0] == 'H':
            # Set thread for subsequent operations
            op = packet[1]
            tid = int(packet[2:])
            self.gdb.send_ack('OK')
        elif packet == 'g':
            # Read general registers.
            regs = await self.uae.read_registers()
            dump = ''.join(regs.as_hex(name) for name in self.__regs__)
            self.gdb.send_ack(dump)
        elif packet == 'G':
            # Write general registers.
            # TODO {G XX…XX}
            self.gdb.send_ack('E00')
        elif packet == 'c':
            # Continue.
            self.uae.resume()
            self.gdb.send_ack()
            return False
        elif packet == 's':
            # Step instruction
            self.uae.step()
            self.gdb.send_ack()
            return False
        elif packet[0] == 'p':
            # Read the value of register n; n is in hex.
            self.gdb.send_ack('DEAD{:04X}'.format(int(packet[1:], 16)))
        elif packet[0] == 'P':
            # Write register n… with value r….
            reg, val = map(lambda x: int(x, 16), packet[1:].split('='))
            await self.uae.write_register(self.__regs__[reg], val)
            self.gdb.send_ack('OK')
        elif packet[0] == 'm':
            # Read length addressable memory units starting at address
            # addr.
            addr, length = map(lambda x: int(x, 16), packet[1:].split(','))
            self.gdb.send_ack(await self.uae.read_memory(addr, length))
        elif packet[0] == 'M':
            # Write length addressable memory units starting at address addr.
            # TODO {M addr,length:XX…}
            self.gdb.send_ack('E00')
        elif packet == 'vCont?':
            self.gdb.send_ack('vCont:')
        elif packet.startswith('z'):
            # Remove _type_ breakpoint/watchpoint starting at _addr_ of _kind_.
            brktype = packet[1]
            brkid = packet[1:]
            addr, kind = map(lambda x: int(x, 16), packet[3:].split(','))
            if brkid in self.brks:
                self.brks.remove(brkid)
                if brktype == '0' or brktype == '1':
                    # Software or hardware breakpoint.
                    await self.uae.remove_breakpoint(addr)
                elif brktype == '2':
                    # Write watchpoint.
                    await self.uae.remove_watchpoint(addr, kind, 'W')
                elif brktype == '3':
                    # Read watchpoint.
                    await self.uae.remove_watchpoint(addr, kind, 'R')
                elif brktype == '4':
                    # Access watchpoint.
                    await self.uae.remove_watchpoint(addr, kind)
                else:
                    raise NotImplementedError(brktype)
                self.gdb.send_ack('OK')
            else:
                self.gdb.send_ack('E01')
        elif packet.startswith('Z'):
            # Insert a software breakpoint at address _addr_ of type _kind_.
            brktype = packet[1]
            brkid = packet[1:]
            addr, kind = map(lambda x: int(x, 16), packet[3:].split(','))
            if brkid not in self.brks:
                self.brks.add(brkid)
                if brktype == '0' or brktype == '1':
                    # Software or hardware breakpoint.
                    await self.uae.insert_breakpoint(addr)
                elif brktype == '2':
                    # Write watchpoint.
                    await self.uae.insert_watchpoint(addr, kind, 'W')
                elif brktype == '3':
                    # Read watchpoint.
                    await self.uae.insert_watchpoint(addr, kind, 'R')
                elif brktype == '4':
                    # Access watchpoint.
                    await self.uae.insert_watchpoint(addr, kind)
                else:
                    raise NotImplementedError(brktype)
                self.gdb.send_ack('OK')
            else:
                self.gdb.send_ack('E01')
        elif packet[0] == 'X':
            # Write data to memory. Memory is specified by its address addr and
            # number of addressable memory units length.
            packet, payload = packet.split(':', 1)
            payload = self.binary_decode(payload)
            addr, length = map(lambda x: int(x, 16), packet[1:].split(','))
            assert length == len(payload)
            hexstr = ''.join('%02x' % c for c in payload)
            await self.uae.write_memory(addr, hexstr)
            self.gdb.send_ack('OK')
        elif packet[0] == 'R':
            # Restart the program being debugged. The XX is ignored.
            # TODO {R XX}
            self.gdb.send_ack('E00')
        elif packet[0] == 'v':
            # The correct reply to an unknown 'v' packet is to return the
            # empty string.
            if packet.startswith('vFile:'):
                self.handle_file(packet.split(':')[1:])
            else:
                self.gdb.send_ack('')

        return True

    async def run(self):
        assert await self.gdb.recv_ack()
        stopdata = await self.uae.prologue()

        running = True

        while running:
            interactive = True

            while interactive:
                packet = await self.gdb.recv_packet()
                if not packet:
                    return

                interactive = await self.handle_packet(packet)
                if interactive:
                    assert await self.gdb.recv_ack()

            uae_stop = asyncio.create_task(self.uae.prologue())
            gdb_wait = asyncio.create_task(self.gdb.recv_break())

            done, pending = await asyncio.wait(
                    {uae_stop, gdb_wait}, return_when=asyncio.FIRST_COMPLETED)

            if uae_stop in done:
                logging.debug('FS-UAE stopped!')
                gdb_wait.cancel()
                try:
                    await gdb_wait
                except asyncio.CancelledError:
                    pass
                stopdata = uae_stop.result()
            else:
                logging.debug('GDB sent break message!')
                self.uae.interrupt()
                stopdata = await uae_stop
                if False:
                    uae_stop.cancel()
                    try:
                        stopdata = await uae_stop
                    except asyncio.CancelledError:
                        self.uae.interrupt()
                        stopdata = await self.uae.prologue()

            # Process stop information.
            regs = stopdata['regs']
            dump = ';'.join('{:x}:{}'.format(num, regs.as_hex(name))
                            for num, name in enumerate(self.__regs__))
            if 'watch' in stopdata:
                dump += ';watch:%x' % stopdata['watch']
            self.gdb.send('T05{};'.format(dump))
            await self.gdb.recv_ack()
