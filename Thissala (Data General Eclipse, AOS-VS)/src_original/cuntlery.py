import struct

def extract_simh_tap(input_file, output_bin):
    with open(input_file, "rb") as f_in, open(output_bin, "wb") as f_out:
        while True:
            # Read SIMH 4-byte record marker (little-endian unsigned int)
            marker = f_in.read(4)
            if not marker or len(marker) < 4:
                break
            record_len = struct.unpack("<I", marker)[0]
            
            # Record length 0 or 0xFFFFFFFF usually denotes a Tape Mark / EOF
            if record_len in (0, 0xFFFFFFFF):
                continue
                
            # Read actual payload block
            data = f_in.read(record_len)
            f_out.write(data)
            
            # Read trailing length byte (SIMH uses twin length markers)
            f_in.read(4)

extract_simh_tap("NADGUG_Library_1996-Jul-02.9trk", "unwrapped_dump.dmp")

