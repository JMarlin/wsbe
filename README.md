# Windowing Systems by Example

You have found your way to the Git repo supporting the blog series [Windowing Systems by Example](https://jmarlin.github.io/wsbe/) that I ~~write over at my blog, trackze.ro~~ keep over here on github pages.

In this repo, you will find a series of numbered folders which correspond to each article. To make life easy, they are all provided with build scripts which use Emscripten, in conjunction with a minimal library abstracting our framebuffer and input drivers, to allow anyone running Windows, OSX, Linux or any other platform that you can get a web browser and/or Emscripten running on to build the code and play with modifying it. Once you have Emscripten installed, all you need to do is run the build script in the folder of the chapter that you're interested in and then open the file runme.html in the root of the repo to see the results.

If you don't have Emscripten on your system yet, [you can head over here and grab the portable version of the SDK for your platform](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The way they package their SDK is lovely, and all you really need to do is download the archive, extract it, run a couple of terminal commands and you should have access to an Emscripten-aware terminal from which you can run these build scripts in just minutes.
