import math
import datetime


def generate_2_axis_gcode(x_series, y_series, input_filename, output_filename, speed=5):
    assert len(x_series) == len(y_series)
    out = ""
    length = 0
    last_x, last_y = x_series[0], y_series[0]

    # Normalize the G-Code to the first point in the CSV
    x_origin_offset = x_series[0]
    y_origin_offset = y_series[0]

    for x, y in zip(x_series, y_series):
        out += f"G1 X{x - x_origin_offset} Y{y - y_origin_offset}\n"
        length += math.sqrt((x - last_x) ** 2 + (y - last_y) ** 2)
        last_x, last_y = x, y
    print(f"Total Length of cut: {length}mm   ==>   ~{length / (speed * 60)} minutes @ {speed}mm/s")

    print(f"Writing G-Code to {output_filename}")
    f = open(output_filename, "w")
    f.write(";FLAVOR:Marlin\n;For: CAT 2-Axis CNC Foamcutter\n")
    f.write(f";Created: {datetime.datetime.now()}\n")
    f.write(f";From: \"{input_filename}\"\n\n")
    f.write(out)
    f.close()
    print(f"Successfully written {output_filename}")
