#ifndef APP_SCREEN_HPP
#define APP_SCREEN_HPP

#include "event_loop.hpp"
#include <chrono>
#include <iosfwd>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

namespace rmioc
{
    class screen;
}

namespace app
{

class screen
{
public:
    screen(
        rmioc::screen& device,
        rfbClient* vnc_client
    );

    event_loop_status event_loop();

    /**
     * Force flushing any pending updates to the screen.
     */
    void repaint();

    /** get x resolution */
    int get_xres();

    /** get x resolution */
    int get_yres();

    /**
     * Available repaint modes.
     */
    enum class repaint_modes
    {
        /**
         * High quality repaints with ~450 ms latency.
         *
         * In this mode, updates are throttled …
         */
        standard = 0,

        /**
         * Black-and-white repaints with ~260 ms latency.
         *
         * Does not clear the flag for pending updates. Is only meant for
         * transitional updates and must be followed by a standard repaint to
         * fully flush pending updates.
         */
        fast = 1
    };

    /** set the rendering mode */
    void set_repaint_mode(repaint_modes mode);

private:
    /** reMarkable screen device. */
    rmioc::screen& device;

    /** VNC connection. */
    rfbClient* vnc_client;

    /**
     * Called by the VNC client library to initialize our local framebuffer.
     *
     * @param client Handle to the VNC client.
     */
    static rfbBool create_framebuf(rfbClient* client);

    /**
     * Called by the VNC client library to register an update from the server.
     *
     * @param client Handle to the VNC client.
     * @param x Left bound of the updated rectangle (in pixels).
     * @param y Top bound of the updated rectangle (in pixels).
     * @param w Width of the updated rectangle (in pixels).
     * @param h Height of the updated rectangle (in pixels).
     */
    static void recv_framebuf(
        rfbClient* client,
        int x, int y, int w, int h
    );

    /** Accumulator for updates received from the VNC server. */
    struct update_info_struct
    {
        /** Left bound of the overall updated rectangle (in pixels). */
        int x = 0;

        /** Top bound of the overall updated rectangle (in pixels). */
        int y = 0;

        /** Width of the overall updated rectangle (in pixels). */
        int w = 0;

        /** Height of the overall updated rectangle (in pixels). */
        int h = 0;

        /** Whether at least one update has been registered. */
        bool has_update = false;
    } update_info;

    /** Last time the reMarkable screen was repainted. */
    std::chrono::steady_clock::time_point last_repaint_time;

    /** Tag used for accessing the instance from C callbacks. */
    static constexpr std::size_t instance_tag = 6803;

    /** Current repaint mode. */
    repaint_modes repaint_mode;
}; // class screen

} // namespace app

#endif // APP_SCREEN_HPP
