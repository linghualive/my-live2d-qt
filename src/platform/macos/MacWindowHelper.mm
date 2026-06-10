#include "MacWindowHelper.h"

#import <Cocoa/Cocoa.h>

static void setLayerTransparent(NSView *view) {
    if (view.layer) {
        view.layer.opaque = NO;
        view.layer.backgroundColor = nil;
    }
    for (NSView *sub in view.subviews) {
        setLayerTransparent(sub);
    }
}

namespace MacWindowHelper {

void makeFullyTransparent(QWidget *window)
{
    if (!window)
        return;

    NSView *nsView = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    if (!nsView)
        return;

    NSWindow *nsWindow = nsView.window;
    if (!nsWindow)
        return;

    nsWindow.opaque = NO;
    nsWindow.hasShadow = NO;
    nsWindow.backgroundColor = [NSColor clearColor];

    setLayerTransparent(nsView);
}

void setIgnoresMouseEvents(QWidget *window, bool ignores)
{
    if (!window)
        return;

    NSView *nsView = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    if (!nsView || !nsView.window)
        return;

    nsView.window.ignoresMouseEvents = ignores;
}

}
