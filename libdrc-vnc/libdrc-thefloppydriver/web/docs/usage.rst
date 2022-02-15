Developing applications using libdrc
====================================

Using the Wii U GamePad in your application
-------------------------------------------

libdrc provides a class called ``drc::Streamer`` in the ``drc/streamer.h``
header file. ``drc::Streamer`` objects expose 5 major functions:

* ``streamer->Start()`` starts streaming data to the Wii U GamePad.
* ``streamer->Stop()`` stops the streamer object.
* ``streamer->PushVidFrame(&pixels, width, height, pixel_format)`` pushes a
  video frame to the Wii U GamePad. ``pixels`` should be a ``vector`` of
  ``width * height`` pixels represented in the given pixel format.
* ``streamer->PushAudSamples(samples)`` pushes audio samples to the Wii U
  GamePad. The API currently expects sound data sampled at 48KHz with two
  interlaced channels (``samples[0]`` is for the left audio channel,
  ``samples[1]`` is for the right audio channel, ...).
* ``streamer->PollInput(&input)`` gets the most recent input data received from
  the GamePad.

Adding basic support for the Wii U GamePad in an already existing application
is a relatively easy task:

* Create a ``drc::Streamer`` global object/singleton and ``Start()`` it when
  starting the program, before the first frame is rendered.
* To handle input from the GamePad, two possibilities are offered to library
  users. The easiest solution is to use the ``SystemInputFeeder`` utility to
  send GamePad data to the operating system (on Linux, via the ``uinput`` API).
  This should allow the application to make use of the GamePad in a transparent
  way if it already has support for other game input devices. To enable this
  utility, use the ``EnableSystemInputFeeder`` method of the streamer object.
  The second, harder way is to write specific input handling code for the
  GamePad using the ``PollInput`` method. This is the only way to get access to
  some features of the GamePad like the touchscreen or the battery status
  informations.
* To stream video, the application needs to provide the raw image pixels to be
  streamed to libdrc. For that, OpenGL software can use ``glReadPixels`` (slow)
  or *Pixel Buffer Objects (PBOs)* in order to fetch the GPU rendering buffer.
  Then, this image can be sent to libdrc via the ``PushVidFrame`` streamer
  function.
* To stream audio, the application needs to provide the raw audio samples to
  libdrc. Audio mixing is usually being done in software, refer to the specific
  audio API documentation to learn how to read the raw audio samples before
  they are sent to the operating system. These samples can then be pushed to
  libdrc via the ``PushAudSamples`` function.
