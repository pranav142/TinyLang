var seed = 5;
var a = 1664525;
var c = 1013904223;
var m = 4294967296;
var count = 0;
var max = 10;

if (seed <= 10) 
    print("please use a seed larger than 10");
else {
    print("Generating pseudorandom numbers:");

    while (count < max) {
        expr seed = (a * seed + c) % m;
        print(seed);
        expr count = count + 1;
    };
};

