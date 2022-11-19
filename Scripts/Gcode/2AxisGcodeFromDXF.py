import math

import ezdxf
import matplotlib.pyplot as plt

dx = 0.1


def plot(x_series, y_series, plot_name):
    max_x, max_y = 0, 0
    for x in x_series:
        max_x = abs(x) if abs(x) > max_x else max_x
    for y in y_series:
        max_y = abs(y) if abs(y) > max_y else max_y

    plt.style.use('dark_background')
    fig, ax = plt.subplots()
    # ax.plot(x_series, y_series, "#ff0000", lw=.1)
    for i in range(1, len(x_series)):
        # ax.plot(x_series[i-1:i+1], y_series[i-1:i+1], f"#{'%06x' % (i // 2)}", lw=.1)
        ax.plot(x_series[i-1:i+1], y_series[i-1:i+1], f"#ff0000", lw=.1)
        print(f"Cords: {(x_series[i-1:i+1], y_series[i-1:i+1])}")
        # print(f"Color: #{'%06x' % (i // 8)}")


    # plt.plot(x_series[0], y_series[0], marker="o", markersize=10, markeredgecolor="#00ff00", markerfacecolor="#00000000")

    ax.set(xlabel='mm', ylabel='mm', title=plot_name)

    ax.grid()

    if max_x > max_y:
        plt.xlim(-max_x * 0.1, max_x + max_x * 0.1)
        plt.ylim((-max_x / 2) - max_x * 0.1, (max_x / 2) + max_x * 0.1)
    else:
        plt.xlim(-max_y * 0.1, max_y * 2 + max_y * 0.1)
        plt.ylim(-max_y * 1.1, max_y * 1.1)

    fig.savefig("test.png", dpi=2000)
    plt.show()


class Shape:
    def __init__(self, points, name):
        self.start = points[0]
        self.end = points[-1]
        self.points = points
        self.name = name


def order_points_from_shapes(shapes):
    # shapes.pop(3)
    print(f"Candidate Shapes: {[f.name for f in shapes]}")
    print(f"Candidate S/E: {[(f.start, f.end) for f in shapes]}")
    largest_dist = 0
    largest_shape = None
    for shape in shapes: # FInd the largest shape
        dist = 0
        for i in range(1, len(shape.points)):
            dist += math.dist(shape.points[0], shape.points[1])
        if dist > largest_dist:
            largest_dist = dist
            largest_shape = shape

    points = []
    shape = largest_shape
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
            min_shape.points.reverse()
        # for p in min_shape.points:
        #     points.append(p)
        shape = min_shape
    print("\n\nOrder:")
    for s in debug_order_list:
        print(s.name)
    return points


def main():
    try:
        doc = ezdxf.readfile("D:\\Projects\\CaseAeronauticsTeam\\CNC_Foamcutter\\Airfoils\\NACA2412 with Hole.DXF")
    except IOError:
        print(f"Not a DXF file or a generic I/O error.")
        exit()
    except ezdxf.DXFStructureError:
        print(f"Invalid or corrupted DXF file.")
        exit()

    # iterate over all entities in modelspace
    msp = doc.modelspace()

    shapes = []

    for e in msp:
        if e.dxftype() == "LINE":
            print("LINE on layer: %s\n" % e.dxf.layer)
            print("start point: %s\n" % e.dxf.start)
            print("end point: %s\n" % e.dxf.end)
            x1 = e.dxf.start[0] * 25.4
            y1 = e.dxf.start[1] * 25.4
            x2 = e.dxf.end[0] * 25.4
            y2 = e.dxf.end[1] * 25.4
            line = Shape([(x1, y1), (x2, y2)], "Line")
            shapes.append(line)
        elif e.dxftype() == "SPLINE":
            print("SPLINE on layer: %s" % e.dxf.layer)
            print(f"Degree {e.dxf.degree}")
            print(f"Control points {e.dxf.n_control_points}")
            print(f"Closed {e.closed}")
            points = []
            for cp in e.control_points:
                points.append((cp[0] * 25.4, cp[1] * 25.4))
            spline = Shape(points, "Spline")
            shapes.append(spline)
        elif e.dxftype() == "ARC":
            print(f"Start Angle {e.dxf.start_angle}")
            print(f"End Angle {e.dxf.end_angle}")
            start_angle = e.dxf.start_angle
            end_angle = e.dxf.end_angle
            center = e.dxf.center
            radius = e.dxf.radius
            dth1 = math.degrees(2 * math.asin(dx / (2 * radius * 25.4)))
            # Always moving counter-clockwise (positive theta)
            if start_angle < end_angle:
                theta = start_angle - end_angle
            else:
                theta = 360 - abs(start_angle - end_angle)
            n_wedges = math.floor(theta / dth1)
            dth2 = math.radians(theta / n_wedges)  # dth1= degrees, dth2 = radians
            print(f"N:{n_wedges}, Theta:{theta}")
            print(f"dth2 rad: {dth2}, dth2 deg: {math.degrees(dth2)}, dth1: {dth1}")
            points = []
            for i in range(n_wedges):
                current_angle = i * dth2 + math.radians(start_angle)
                print(f"Current Angle: {current_angle}")
                print(f"x: {25.4 * radius * math.cos(current_angle) + center[0] * 25.4}")
                points.append((25.4 * radius * math.cos(current_angle) + center[0] * 25.4,
                               25.4 * radius * math.sin(current_angle) + center[1] * 25.4))
            arc = Shape(points, "Arc")
            shapes.append(arc)

    ordered_points = order_points_from_shapes(shapes)
    # ordered_points = shapes[0].points
    x = [p[0] for p in ordered_points]
    y = [p[1] for p in ordered_points]

    plot(x, y, "Test")

    print("==========================")
    for e in msp:
        print(e.dxftype())
    # entity query for all LINE entities in modelspace
    # for e in msp.query("LINE"):
    #     print_entity(e)


if __name__ == '__main__':
    main()
