echo(version=version());

fan_width=75;
fan_height=15;


// simple 2D -> 3D extrusion of a rectangle
color("red")
rotate(a=[0,-90,0])
        rotate_extrude(angle=90)
        translate([(fan_height+5)/2,0,0]) difference() {
           square([fan_height+5, fan_width+5], center = true);
           square([fan_height, fan_width], center = true);
        };

color("blue")
rotate(a=[90,-90,0])
        linear_extrude(height=10)
    translate([(fan_height+5)/2,0,0]) difference() {
           square([fan_height+5, fan_width+5], center = true);
           square([fan_height, fan_width], center = true);
        }