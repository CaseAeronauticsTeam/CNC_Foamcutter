import math

import ezdxf
import matplotlib.pyplot as plt

from gcode_writer import generate_2_axis_gcode

dx = 0.1


def plot(x_series, y_series, plot_name, cut_from_trailing_edge=False):
    x_origin_offset = x_series[0]
    y_origin_offset = y_series[0]
    max_x, max_y = max(x_series), max(y_series)
    min_x, min_y = min(x_series), min(y_series)
    x_chord = max_x - min_x
    y_chord = max_y - min_y  # I know this isn't really chord... sue me
    # for x in x_series:
    #     max_x = abs(x) if abs(x) > max_x else max_x
    # for y in y_series:
    #     max_y = abs(y) if abs(y) > max_y else max_y

    print(f"Plot Offset: {(x_origin_offset, y_origin_offset)}")

    if not cut_from_trailing_edge:
        max_x -= x_origin_offset
        max_y -= y_origin_offset

    plt.style.use('dark_background')
    fig, ax = plt.subplots()
    # ax.plot(x_series, y_series, "#ff0000", lw=.1)
    for i in range(1, len(x_series)):
        x1, x2 = x_series[i - 1:i + 1]
        y1, y2 = y_series[i - 1:i + 1]
        x1 -= x_origin_offset
        x2 -= x_origin_offset
        y1 -= y_origin_offset
        y2 -= y_origin_offset
        # ax.plot(x_series[i-1:i+1], y_series[i-1:i+1], f"#{'%06x' % (i // 2)}", lw=.1)
        ax.plot((x1, x2), (y1, y2), f"#ff0000", lw=.2)
        plt.arrow(x1, y1, x2 - x1, y2 - y1, shape='full', lw=0, length_includes_head=True, head_width=1)
        # print(f"Cords: {((x1, x2), (y1, y2))}")
        # print(f"Color: #{'%06x' % (i // 8)}")

    plt.plot(x_series[0] - x_origin_offset, y_series[0] - y_origin_offset, marker="o", markersize=10,
             markeredgecolor="#00ff00", markerfacecolor="#00000000")

    ax.set(xlabel='mm', ylabel='mm', title=plot_name)

    ax.grid()


    print(f"Chord: {x_chord}\nThickenss: {y_chord}")
    if cut_from_trailing_edge:
        if x_chord > y_chord:
            plt.xlim(-x_chord * 0.1 - x_chord, x_chord * 0.1)
            plt.ylim((-x_chord / 2) - x_chord * 0.1, (x_chord / 2) + x_chord * 0.1)
        else:
            plt.xlim(-y_chord * 0.1, y_chord * 2 + y_chord * 0.1)
            plt.ylim(-y_chord * 1.1, y_chord * 1.1)
    else:
        if x_chord > y_chord:
            plt.xlim(-x_chord * 0.1, x_chord + x_chord * 0.1)
            plt.ylim((-x_chord / 2) - x_chord * 0.1, (x_chord / 2) + x_chord * 0.1)
        else:
            plt.xlim(-y_chord * 0.1, y_chord * 2 + y_chord * 0.1)
            plt.ylim(-y_chord * 1.1, y_chord * 1.1)

    fig.savefig("preview.png", dpi=2000)
    plt.show()


class Shape:
    def __init__(self, points, name):
        self.start = points[0]
        self.end = points[-1]
        self.points = points
        self.name = name

    def reverse(self):
        self.points.reverse()
        self.start = self.points[0]
        self.end = self.points[-1]


def get_shapes_from_msp(msp):
    shapes = []
    for e in msp:
        if e.dxftype() == "LINE":
            print("LINE on layer: %s" % e.dxf.layer)
            print("\tstart point: %s" % e.dxf.start)
            print("\tend point: %s\n" % e.dxf.end)
            x1 = e.dxf.start[0] * 25.4
            y1 = e.dxf.start[1] * 25.4
            x2 = e.dxf.end[0] * 25.4
            y2 = e.dxf.end[1] * 25.4
            line = Shape([(x1, y1), (x2, y2)], "Line")
            shapes.append(line)
        elif e.dxftype() == "SPLINE":
            print("SPLINE on layer: %s" % e.dxf.layer)
            print(f"\tDegree {e.dxf.degree}")
            print(f"\tControl points {e.dxf.n_control_points}")
            print(f"\tClosed: {e.closed}\n")
            points = []
            for cp in e.control_points:
                points.append((cp[0] * 25.4, cp[1] * 25.4))
            spline = Shape(points, "Spline")
            shapes.append(spline)
        elif e.dxftype() == "ARC":
            print("ARC on layer: %s" % e.dxf.layer)
            print(f"\tStart Angle {e.dxf.start_angle}")
            print(f"\tEnd Angle {e.dxf.end_angle}")
            start_angle = e.dxf.start_angle
            end_angle = e.dxf.end_angle
            center = e.dxf.center
            radius = e.dxf.radius
            print(f"\tRadius: {radius}")
            # dth1 = smallest unit of angular precision that corresponds exactly to the linear precision,
            #        while truncating excess angle, i.e., this will result in the arc not ending where intended
            dth1 = math.degrees(2 * math.asin(dx / (2 * radius * 25.4)))
            # Always moving counter-clockwise (positive theta)
            if start_angle < end_angle:
                theta = start_angle - end_angle
            else:
                theta = 360 - abs(start_angle - end_angle)
            n_wedges = math.floor(theta / dth1)
            # dth2 = slightly larger than dth2 that extends the arc to end where intended
            dth2 = math.radians(theta / n_wedges)  # dth1 = degrees, dth2 = radians
            print(f"\tN:{n_wedges}, Theta:{theta}")
            print(f"\tdth2 rad: {dth2}, dth2 deg: {math.degrees(dth2)}, dth1: {dth1}\n")
            points = []
            for i in range(n_wedges):
                current_angle = i * dth2 + math.radians(start_angle)
                # print(f"Current Angle: {current_angle}")
                # print(f"x: {25.4 * radius * math.cos(current_angle) + center[0] * 25.4}")
                points.append((25.4 * radius * math.cos(current_angle) + center[0] * 25.4,
                               25.4 * radius * math.sin(current_angle) + center[1] * 25.4))
            arc = Shape(points, "Arc")
            shapes.append(arc)
        # Ignore TEXT objects
    return shapes


def order_points_from_shapes(shapes):
    # shapes.pop(3)
    print(f"Candidate Shapes: {[f.name for f in shapes]}")
    print(f"Candidate S/E: {[(f.start, f.end) for f in shapes]}")
    largest_dist = 0
    largest_shape = None
    for shape in shapes:  # Find the largest shape
        dist = 0
        for i in range(1, len(shape.points)):
            dist += math.dist(shape.points[0], shape.points[1])
        if dist > largest_dist:
            largest_dist = dist
            largest_shape = shape
    print(f"Largest Shape: {largest_shape.name} {(largest_shape.start, largest_shape.end)}\n")

    points = []
    shape = largest_shape  # Start ordering with the largest shape
    shapes_to_draw = shapes.copy()
    debug_order_list = []
    for _ in range(len(shapes)):
        shapes_to_draw.remove(shape)
        debug_order_list.append(shape)
        print(f"Adding: {shape.name}")
        for p in shape.points:
            points.append(p)
        min_dist = 9e99
        min_shape = None
        is_end = False
        if len(shapes) > 1:
            for sh in shapes_to_draw:
                if math.dist(shape.end, sh.start) < min_dist:
                    min_dist = math.dist(shape.end, sh.start)
                    min_shape = sh
                    is_end = False
                if math.dist(shape.end, sh.end) < min_dist:
                    min_dist = math.dist(shape.end, sh.end)
                    min_shape = sh
                    is_end = True
                # print(f"Distances: {math.dist(shape.end, sh.start), math.dist(shape.end, sh.end)}")
                # print(f"Shape: {min_shape.name}")
        else:
            min_shape = shape
        if is_end:
            min_shape.reverse()
        # for p in min_shape.points:
        #     points.append(p)
        shape = min_shape
    print("\n\nOrder:")
    for s in debug_order_list:
        print(s.name)
    return points


def generate_gcode_from_dxf(input_filename, output_filename, speed, show_plot=True, reverse=False, cut_from_trailing_edge=False):
    try:
        doc = ezdxf.readfile(input_filename)
    except IOError:
        print(f"Not a DXF file or a generic I/O error.")
        exit()
    except ezdxf.DXFStructureError:
        print(f"Invalid or corrupted DXF file.")
        exit()

    msp = doc.modelspace()

    print("DXF Contents:")
    for e in msp:
        print('\t' + e.dxftype())
    print("\n\n")

    shapes = get_shapes_from_msp(msp)

    ordered_points = order_points_from_shapes(shapes)

    print(f"Points:\n{ordered_points}")

    # Find left most point and rotate list to make that the starting point
    min_or_max_x = ordered_points[0][0]
    start_index = 0
    for i, p in enumerate(ordered_points):
        if cut_from_trailing_edge:
            if p[0] > min_or_max_x:
                min_or_max_x = p[0]
                start_index = i
        else:
            if p[0] < min_or_max_x:
                min_or_max_x = p[0]
                start_index = i
    if cut_from_trailing_edge:
        print(f"Max_x: {min_or_max_x}, Start Index: {start_index}")
    else:
        print(f"Min_x: {min_or_max_x}, Start Index: {start_index}")
    ordered_points = ordered_points[start_index:] + ordered_points[:start_index]

    if reverse:
        ordered_points.reverse()

    # Check to make sure list of points is a closed loop
    if ordered_points[0] != ordered_points[-1]:
        ordered_points.append(ordered_points[0])

    x = [p[0] for p in ordered_points]
    y = [p[1] for p in ordered_points]

    if show_plot:
        plot(x, y, input_filename.split('\\')[-1], cut_from_trailing_edge=cut_from_trailing_edge)

    generate_2_axis_gcode(x, y, input_filename, output_filename, speed)


if __name__ == '__main__':
    generate_gcode_from_dxf(
        # "D:\\Projects\\CaseAeronauticsTeam\\CNC_Foamcutter\\Airfoils\\OLD_AIRFOILS\\FS_x-01-001 NACA2412 foam wing DXF inner half.DXF",
        "D:\\Projects\\CaseAeronauticsTeam\\CNC_Foamcutter\\Airfoils\\FSx-01-002 NACA2412 left foam wing.DXF",
        "out.gcode", 0.6666, show_plot=True, cut_from_trailing_edge=True)
