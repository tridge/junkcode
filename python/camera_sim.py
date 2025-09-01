#!/usr/bin/env python3
'''
simulate a RTSP camera with a clock display
'''

'''
  dependencies install:

  sudo apt-get install -y \
  python3-gi gir1.2-gst-rtsp-server-1.0 \
  gstreamer1.0-tools gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly gstreamer1.0-libav
'''

import argparse
import signal
import sys

# GStreamer / RTSP server
try:
    import gi
    gi.require_version("Gst", "1.0")
    gi.require_version("GstRtspServer", "1.0")
    from gi.repository import Gst, GstRtspServer, GObject, GLib
except Exception as e:
    print("Error: GStreamer RTSP Python bindings not available.", file=sys.stderr)
    print("Hint (Debian/Ubuntu): sudo apt-get install python3-gi gir1.2-gst-rtsp-server-1.0", file=sys.stderr)
    raise

def build_pipeline(width: int, height: int, fps: int) -> str:
    """
    Build the GStreamer pipeline string used by the RTSP media factory.
    Produces a black background with a centered HH:MM:SS clock overlaid,
    encoded as H.264 and packetized for RTP.
    """
    # Keyframe interval (GOP) ~ 2 seconds to make players happy when joining mid-stream
    keyint = max(fps * 2, 2)
    # Keep bitrate modest; tweak as desired
    bitrate_kbps = 1200

    # Notes:
    # - videotestsrc is-live=true ensures live timestamps
    # - caps set resolution and frame rate
    # - clockoverlay draws the ticking clock (strftime format)
    # - x264enc tuned for low-latency
    # - h264parse with config-interval=-1 ensures SPS/PPS are sent regularly
    # - rtph264pay name=pay0 is required by gst-rtsp-server
    pipe = (
        f"videotestsrc is-live=true pattern=black ! "
        f"video/x-raw,format=I420,width={width},height={height},framerate={fps}/1 ! "
        f"clockoverlay time-format=\"%H:%M:%S\" halignment=center valignment=center "
        f"shaded-background=true font-desc=\"Sans, 64\" ! "
        f"x264enc tune=zerolatency speed-preset=ultrafast bitrate={bitrate_kbps} "
        f"key-int-max={keyint} qp-min=18 qp-max=35 byte-stream=true ! "
        f"h264parse config-interval=-1 ! "
        f"rtph264pay name=pay0 pt=96"
    )
    return pipe

class ClockRTSPServer:
    def __init__(self, address: str, port: int, mount: str, width: int, height: int, fps: int):
        self.address = address
        self.port = port
        self.mount = mount
        self.width = width
        self.height = height
        self.fps = fps

        # Initialize GStreamer
        Gst.init(None)

        # Create server
        self.server = GstRtspServer.RTSPServer.new()
        self.server.set_address(self.address)
        self.server.set_service(str(self.port))

        self.factory = GstRtspServer.RTSPMediaFactory.new()
        self.factory.set_launch(build_pipeline(width, height, fps))
        # Shared = one encoder instance shared by all clients
        self.factory.set_shared(True)

        mnt = self.server.get_mount_points()
        mnt.add_factory(self.mount, self.factory)

        self.loop = GLib.MainLoop()
        self._attach_id = None

    def start(self):
        self._attach_id = self.server.attach(None)
        print(f"RTSP clock camera running:")
        print(f"  rtsp://{self.address or '0.0.0.0'}:{self.port}{self.mount}")
        print(f"  {self.width}x{self.height} @ {self.fps} fps (H.264)")
        try:
            self.loop.run()
        except KeyboardInterrupt:
            pass

    def stop(self):
        if self._attach_id is not None:
            GLib.source_remove(self._attach_id)
            self._attach_id = None
        self.loop.quit()

def positive_int(value: str) -> int:
    iv = int(value)
    if iv <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return iv

def main():
    parser = argparse.ArgumentParser(
        description="Simulated RTSP network camera streaming H.264 with a ticking clock overlay."
    )
    parser.add_argument("--host", default="0.0.0.0",
                        help="IPv4 address to bind (default: 0.0.0.0)")
    parser.add_argument("--port", type=positive_int, default=8554,
                        help="RTSP TCP port to listen on (default: 8554)")
    parser.add_argument("--width", type=positive_int, default=1280,
                        help="Video width (default: 1280)")
    parser.add_argument("--height", type=positive_int, default=720,
                        help="Video height (default: 720)")
    parser.add_argument("--fps", type=positive_int, default=30,
                        help="Frames per second (default: 30)")
    parser.add_argument("--mount", default="/stream",
                        help="RTSP mount point (default: /stream)")
    args = parser.parse_args()

    # Basic sanity
    if args.mount[0] != "/":
        print("Error: --mount must start with '/'. For example: /stream", file=sys.stderr)
        sys.exit(2)

    server = ClockRTSPServer(
        address=args.host,
        port=args.port,
        mount=args.mount,
        width=args.width,
        height=args.height,
        fps=args.fps
    )

    # Clean shutdown on SIGTERM/SIGINT
    def _handle_sigterm(*_):
        server.stop()
    signal.signal(signal.SIGTERM, _handle_sigterm)
    signal.signal(signal.SIGINT, _handle_sigterm)

    server.start()

if __name__ == "__main__":
    main()
