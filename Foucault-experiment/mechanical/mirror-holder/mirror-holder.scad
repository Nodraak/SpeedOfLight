/*
    Mirror holder
*/

$fn = 72;

echo("==========");

//
// Helpers
//

function r2d(r) = r * 180 / 3.14159;
function d2r(d) = d * 3.14159 / 180;

pi_deg = acos(-1);
echo("pi_deg", pi_deg);

delta = 99;

//
// Constants
//

d_main = 40; // arbitrary
r_main = d_main / 2;

mirror_L = 10; // space
mirror_D = 20; // spec
mirror_H = 30; // spec

d_hole = 3; // spec
h_hole = 12.5; // spec
h_hole_margin = 2.5 + 5;

h_main = mirror_H + 2 * (h_hole + h_hole_margin);
echo("h_main", h_main);

laser_W = sqrt(5*5 + 5*5);
laser_L = delta;
laser_offset = 5;

rear_cap_h = 4.12; // manual optim COG
rear_W = d_main;
rear_H = h_main + delta;

// rear_cap_h 4.15 = COG base estimated 1.01, reality 1.03
// => better rear_cap_h 4.12 (-0.006)

//
// Main cylinder
//

difference() {
    cylinder(h = h_main, d = d_main, center = true);

    // Motor holes
    m_h = mirror_H/2 + h_hole_margin + h_hole/2;
    translate([0, 0, m_h]) cylinder(h = h_hole+1, d = d_hole, center = true);
    translate([0, 0, -m_h]) cylinder(h = h_hole+1, d = d_hole, center = true);

    // Mirror
    cube([mirror_L, mirror_D, mirror_H], center = true);

    // Laser path - center
    rotate([0, 0, 45]) translate([0, -laser_W/2, -mirror_H/2]) cube([laser_L, laser_W, mirror_H]);
    rotate([0, 0, -45]) translate([0, -laser_W/2, -mirror_H/2]) cube([laser_L, laser_W, mirror_H]);

    // Laser path - offset
    translate([0, laser_offset, 0])
        rotate([0, 0, 45]) translate([0, -laser_W/2, -mirror_H/2]) cube([laser_L, laser_W, mirror_H]);
    translate([0, -laser_offset, 0])
        rotate([0, 0, -45]) translate([0, -laser_W/2, -mirror_H/2]) cube([laser_L, laser_W, mirror_H]);

    // Rear balancing
    translate([-rear_W-r_main+rear_cap_h, -rear_W/2, -rear_H/2]) cube([rear_W, rear_W, rear_H]);

    // DEBUG: cut in half
    // translate([-50, -50, 0])
    //    cube([100, 100, 100]);
}

//
// Check COG
//

// cog_mirror
cog_mirror = -1.36;

// cog_base
theta_deg = 2*acos((r_main -rear_cap_h)/r_main );
// The centroid x of the remaining shape (signed toward the side opposite the removed cap)
cog_base = 2 * r_main * sin(theta_deg / 2)^3 / (3 * (3.14159 - 0.5*(d2r(theta_deg)-sin(theta_deg))));
echo("cog_base", cog_base);

// cog_base = 1.03;

// result
cog_global = (2 * (h_hole + h_hole_margin) * cog_base + mirror_H * cog_mirror) / h_main;

echo("Estimated COG:", cog_global);

echo("==========");
