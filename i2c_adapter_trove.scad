module support(x, y, ang, length, height) {
	border=2;
	translate([x,y,0])
	rotate(a=ang)
		union() {
			polyhedron(
				points=[ [0,0,0], [0,length,0], [length,0,0],
  	                [0,0,height], [0,length,height], [length,0,height] ],
				triangles=[ [2,1,0], [3,4,5], 
  	                   [1,5,4], [2,5,1],
								[2,0,5], [0,3,5],
								[4,3,0], [0,1,4] ]
			);
			hull() {
				translate([length, -border/2, 0])
					cylinder(r=border/2, h=7);
				translate([-border/2, -border/2, 0])
					cylinder(r=border/2, h=7);
			}
			hull() {
				translate([-border/2, length, 0])
					cylinder(r=border/2, h=7);
				translate([-border/2, -border/2, 0])
					cylinder(r=border/2, h=7);
			}
		}
}

module snapper(x,y, height, length, ang) {
	translate([x,y,0])
	rotate(a=ang)
	translate([-length/2,-1,0])
		cube(size=[length,1,height]);

	translate([x,y,height-1])
	rotate(a=ang)
	translate([-length/2,0,0])
		difference() {
			cube(size=[length,1,1]);
			translate([0,1,0])
			rotate([45,0,0])
				cube(size=[length,2,2]);
		}
}

module trove(xsize, ysize) {
	height = 7;
	border = 2;

	translate([border,border,0])
	union() {
		// frame
		translate([-border/2, -border/2, 0])
			cube(size=[xsize+border, ysize+border, 1]);
/*		difference() {
			hull() {
				translate([0,0,0])
					cylinder(r=border, h=height);
				translate([xsize,,0])
					cylinder(r=border, h=height);
				translate([0,ysize,0])
					cylinder(r=border, h=height);
				translate([xsize,ysize,0])
					cylinder(r=border, h=height);
			}
			translate([0,0,1])
				cube(size=[xsize,ysize,height-1]);

			translate([10,0-border,1])
				cube(size=[xsize-20,ysize+2*border,height-1]);

			translate([0-border,10,1])
				cube(size=[xsize+2*border,ysize-20,height-1]);

		}*/

 	   // auflagen
		support(0,0, 0, 5, 4);
		support(xsize,0, 90, 5, 4);
		support(0,ysize, 270, 5, 4);
		support(xsize,ysize, 180, 5, 4);

		//nubsies
		snapper(0, (ysize/2), height, 4, 270);
		snapper(xsize, (ysize/2), height-1, 4, 90);
		snapper((xsize/2), 0, height-1, 4, 0);
		snapper((xsize/2), ysize, height-1, 4, 180);
	}
}

module trove_with_hole(xsize, ysize, xpos, ypos, diameter) {
	rad = diameter/2;
	difference() {	
		union() {
			trove(xsize, ysize);
			translate([xpos,ypos,0])
			cylinder(h=3.5, r=rad+3);	
		}	
		translate([xpos,ypos,0])
			cylinder(h=3.5, r=rad);
		translate([xpos,ypos,2])
			cylinder(h=1.5, r=rad+2);
	}
}


$fn=50;
trove_with_hole(46,43, 40,10,4.1);
