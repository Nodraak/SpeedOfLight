/*
    # Main assembly

    Initial ChatGPT prompt:

    ```
    Write an openscad script code:

    * Create a rectangular box w=170 x d=80 x h=80. Write w=170 as 10+30+90+30+10
    * Difference a cylinder, centered in the rectangular box, of diameter=5+40+5
    * Difference a smaller rectangular box, depth=5+40+5, height=(80-(5+40+5))/2 + 5 + 5, just below the cylinder
    * Difference 4 holes on each side of the rectangular box, around the cylinder hole. diameter=3, d=10 further away than the cylinder's diameter.
    * Difference a vertical shaft (cylinder) from the rectangular box; d=10, centered on the depth axis, at an horizontal position of 10+30+90*1/4
    * Difference an horizontal shaft (cylinder) from the rectangular box; d=10, centered on the vertical axis, at an horizontal position of 10+30+90*1/4
    * Difference a vertical shaft (cylinder) from the rectangular box; d=10, centered on the depth axis, at an horizontal position of 10+30+90*3/4
    * Difference an horizontal shaft (cylinder) from the rectangular box; d=10, centered on the vertical axis, at an horizontal position of 10+30+90*3/4

    A few requests:

    * Write only the code and nothing else.
    * Do not add comments for obvious things, but do add a comment for each step above.
    * Use variables as needed for constants.
    * Add "$fn = 60;" at the top.
    ```
*/

// number of segments ; overrides $fa (minimum angle) and $fn (minimum size)
$fn = $preview ? 32 : 64;

//
// Constants
//

// main_box_w = 10 + 30 + 90 + 30 + 10;  // wall + motor + spinning cylinder + motor + wall
main_box_w = 10 + 30-10 + 70 + 30-10 + 10;  // wall + motor-wall + spinning cylinder + motor-wall + wall
main_box_h = 80;

main_cyl_d = 2 + 40 + 2; // 5 mm margin around spinning cylinder

motor_cube_depth = 20;
motor_cube_side = 60;

motor_screw_hole_pos = 40;
motor_screw_hole_d = 3;

// light_shaft_pos = 90*1/4;
light_shaft_pos = 40*1/4;
light_shaft_d = 10;
light_shaft_d_2 = 3;

laser_l = 45;  // 35 + some margin for the wire
laser_d = 13;  // 12 is too small
laser_box_d = laser_d + 2*4;

camera_pos = 40*1/4;
camera_holder_side = 30;
camera_holder_height = 45;

//
// Top assembly
//

module top_assembly() {
    translate([0, 0, main_box_h/2]) {
        // main plate
        difference() {

            // Create main plate
            translate([-main_box_w/2, -main_box_h/2, 0])
                cube([main_box_w, main_box_h, 10]);

            // 4 resting slots
            translate([0, 0, -main_box_h/2])
                bottom_assembly();

            // Vertical shaft at -1/4
            translate([-light_shaft_pos, 0, 0])
                cylinder(d = light_shaft_d_2, h = main_box_h + 2, center = true);

            // Vertical shaft at +1/4
            translate([+light_shaft_pos, 0, 0])
                cylinder(d = light_shaft_d_2, h = main_box_h + 2, center = true);

            // 4 screw holes - top
            for (x_offset = [-(main_box_w/2-5+1), +main_box_w/2-5+1])
                for (y_offset = [-(main_box_h/2-5+1), +main_box_h/2-5+1])
                    translate([x_offset, y_offset, 5])
                        cylinder(d = motor_screw_hole_d, h = 10+1, center = true);
        }

        // laser
        difference() {
            // laser holder
            translate([-light_shaft_pos, 0, laser_l/2])
                cylinder(d = laser_box_d, h = laser_l, center = true);

            // laser hole
            translate([-light_shaft_pos, 0, laser_l/2])
                cylinder(d = laser_d, h = laser_l+1, center = true);

            // wire
            translate([-light_shaft_pos-laser_box_d/2, 0, laser_l])
                cube([laser_box_d, 5, 2*5], center = true);
        }

        // camera
        difference() {
            // main block
            translate([+camera_pos, 0, laser_l/2])
                cube([camera_holder_side, camera_holder_side, camera_holder_height], center = true);

            // view hole
            translate([+camera_pos, 0, laser_l/2])
                cylinder(d = light_shaft_d, h = camera_holder_height+1, center = true);

            // laser hole
            translate([-light_shaft_pos, 0, laser_l/2])
                cylinder(d = laser_d, h = laser_l+1, center = true);
        }
    }
}

//
// Bottom assembly
//

module bottom_assembly() {
    difference() {

        // Create main rectangular box
        cube([main_box_w, main_box_h, main_box_h], center = true);

        rotate([0, 90, 0]) {
            // Difference main centered horizontal cylinder
            cylinder(d = main_cyl_d, h = main_box_w + 2, center = true);

            // centered horizontal rectangular box below cylinder
            translate([0, -main_cyl_d/2, -(main_box_w/2 - 10)])
                cube([main_box_h/2 + 1, main_cyl_d, main_box_w - 2*10]);

            // Sides motors attachement - cube
            translate([0, 0, -main_box_w/2 + 10 + motor_cube_depth/2 - 1])
                cube([motor_cube_side, motor_cube_side, motor_cube_depth], center = true);
            translate([0, 0, main_box_w/2 - 10 - motor_cube_depth/2])
                cube([motor_cube_side, motor_cube_side, motor_cube_depth], center = true);

            // 4 screw holes - on each side around cylinder hole
            for (x_offset = [-main_box_w/2, +main_box_w/2])
                for (y_offset = [-motor_screw_hole_pos/2, +motor_screw_hole_pos/2])
                    for (z_offset = [-motor_screw_hole_pos/2, +motor_screw_hole_pos/2])
                        translate([z_offset, y_offset, x_offset])
                            cylinder(d = motor_screw_hole_d, h = 2*10+1, center = true);
        }

        // Vertical shaft at -1/4
        translate([-light_shaft_pos, 0, 0])
            cylinder(d = light_shaft_d, h = main_box_h + 2, center = true);
        // Horizontal shaft at -1/4
        translate([-light_shaft_pos, -main_box_h/2, 0])
            rotate([90, 0, 0])
                cylinder(d = light_shaft_d, h = main_box_h + 2, center = true);

        // Vertical shaft at +1/4
        translate([+light_shaft_pos, 0, 0])
            cylinder(d = light_shaft_d, h = main_box_h + 2, center = true);
        // Horizontal shaft at +1/4
        translate([+light_shaft_pos, -main_box_h/2, 0])
            rotate([90, 0, 0])
                cylinder(d = light_shaft_d, h = main_box_h + 2, center = true);

        // resting slot - front
        translate([0, -main_box_h/2, 0])
            cube([40, 2*5, 20], center = true);

        // 4 resting slots - top
        for (x_offset = [-(main_box_w/2-5+1), +main_box_w/2-5+1])
            for (y_offset = [-(main_box_h/2-5+1), +main_box_h/2-5+1])
                translate([x_offset, y_offset, main_box_h/2-5])
                    cylinder(d = motor_screw_hole_d, h = 10+1, center = true);

        // cutout - side
        // translate([50, 0, 0])
        //     cube([100, 100, 100], center = true);

        // cutout - front
        // translate([0, -50, 0])
        //     cube([200, 100, 100], center = true);
    }
}

//
// Main
//

top_assembly();
bottom_assembly();
