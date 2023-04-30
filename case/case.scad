wand=1;
innen_=[89,30,23];
deckel_=3;
module innen(){cube(innen_);}
module aussen(){translate(-wand*[1,1,1]) cube(innen_+2*wand*[1,1,1]);}
module lan(){translate([70,9,4]) cube([22,19,15]);}
module kabel(){translate([25,-3,innen_[2]-deckel_]) cube([1,24,50]);}
module gehaeuse(){difference() {
    aussen();
    innen();
    lan();
    kabel();
};}
module boden_mask(){
    translate([0,0,-deckel_])
    translate(-2*wand*[1,1,0])
    cube(innen_+wand*[4,4,0]);
}
module boden(){
    intersection(){
        gehaeuse();
        boden_mask();
    }
}
module deckel_mask(){
    translate([0,0,innen_[2]-deckel_+0.1])
    translate(-2*wand*[1,1,0])
    cube(innen_+wand*[4,4,0]);
}
module deckel(){
    intersection(){
        gehaeuse();
        deckel_mask();
    }
}
module halter(){
    rotate([90,0,90])
    linear_extrude(height=3,center=true)
    polygon([[-0.1,0],[1,0],[1,-5-deckel_],[0.2,-5-deckel_],[-1,-4-deckel_],[-0.1,-4-deckel_]]);
}
module deckel_mit_halter(){
    union(){
        deckel();
        translate([15,0,innen_[2]]) halter();
        translate([60,0,innen_[2]]) halter();
        translate([15,innen_[1],innen_[2]]) rotate([0,0,180]) halter();
        translate([60,innen_[1],innen_[2]]) rotate([0,0,180]) halter();
    }
}
module boden_mit_halter(){
    difference(){
        boden();
        translate([15,0,innen_[2]-4.5-deckel_])
        cube([3.5,4*innen_[1],1.5],center=true);
        translate([60,0,innen_[2]-4.5-deckel_])
        cube([3.5,4*innen_[1],1.5],center=true);
    }
}