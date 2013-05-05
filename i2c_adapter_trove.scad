 module trove(xsize, ysize) {
	difference() {
		minkowski() {
			translate([2,2,0])
				cube(size=[xsize,ysize,3]);
			//translate([2,2,0])
				cylinder(r=2, h=3);
		}
		translate([2,2,1])
			cube(size=[xsize,ysize,5]);
	}
   // auflagen
	translate([2,2,0])
		cube(size=[5,5,4]);
	translate([xsize-3,2,0])
		cube(size=[5,5,4]);
	translate([2,ysize-3,0])
		cube(size=[5,5,4]);
	translate([xsize-3,ysize-3,0])
		cube(size=[5,5,4]);

	//nubsies
	translate([2, (ysize/2)-2, 5])
		cube(size=[1,4,1]);
	translate([xsize+1, (ysize/2)-2, 5])
		cube(size=[1,4,1]);
}

module trove_with_hole(xsize, ysize, xpos, ypos) {
	difference() {	
		union() {
			trove(xsize, ysize);
			translate([xpos,ypos,0])
			cylinder(h=4, r=10);	
		}	
		translate([xpos,ypos,0])
			cylinder(h=4, r=4);
		translate([xpos,ypos,2])
			cylinder(h=2, r=8);
	}
}

trove_with_hole(43,46, 37,10);