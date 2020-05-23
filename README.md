# Poolpaper

In 2013 it looked like this: https://www.youtube.com/watch?v=h65P0QYTLIMo but I couldn't keep up with Android's moving goalposts.

All physical aspects of the optics are there but the water motion is just the simple harmonic approximation. In reality water waves are propogated by both gravity and surface tension with different behaviour. I think that could be modelled on a modern phone to make a completely realistic simulation.

There's a problem that errors coming from the large grid spacing cause positive feedback and ringing in waves whose wavelength is double the grid spacing. These can be combated with blur but at that time the hardware limited me to a huge grid spacing and the bug couldn't be eliminated without losing most of the attraction.

This code calculates the water motion on the CPU but nowadays that should be done with OpenCL.

It was also great for keeping your hands warm on frosty mornings.

