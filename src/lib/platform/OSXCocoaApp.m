/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "platform/OSXCocoaApp.h"

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

NSWindow *dummyWindow = NULL;

// All overlay work is asynchronous on the UI thread, away from input injection.
static NSWindow *entryFlash;
static NSWindow *entryMarker;
static NSUInteger entryGeneration;

@interface DeskflowEntryRing : NSView
@end

@implementation DeskflowEntryRing
- (void)drawRect:(NSRect)dirtyRect
{
  [[NSColor clearColor] setFill];
  NSRectFillUsingOperation(self.bounds, NSCompositingOperationCopy);
  NSBezierPath *ring = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(self.bounds, 5, 5)];
  [[NSColor blackColor] setStroke];
  [ring setLineWidth:6];
  [ring stroke];
  [[NSColor colorWithCalibratedRed:0.2 green:0.85 blue:1 alpha:1] setStroke];
  [ring setLineWidth:3];
  [ring stroke];
}
@end

static NSWindow *entryWindow(NSRect frame)
{
  NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                               styleMask:NSWindowStyleMaskBorderless
                                                 backing:NSBackingStoreBuffered defer:NO];
  [window setReleasedWhenClosed:NO];
  [window setOpaque:NO];
  [window setBackgroundColor:[NSColor clearColor]];
  [window setHasShadow:NO];
  [window setIgnoresMouseEvents:YES];
  [window setLevel:NSStatusWindowLevel];
  [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                               NSWindowCollectionBehaviorFullScreenAuxiliary |
                               NSWindowCollectionBehaviorIgnoresCycle];
  return window;
}

static void clearEntryCue(void)
{
  [entryFlash close];
  [entryFlash release];
  entryFlash = nil;
  [entryMarker close];
  [entryMarker release];
  entryMarker = nil;
}

void hideScreenEntryCue()
{
  dispatch_async(dispatch_get_main_queue(), ^{
    ++entryGeneration;
    clearEntryCue();
  });
}

void showScreenEntryCue(double x, double y)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    const NSUInteger generation = ++entryGeneration;
    clearEntryCue();
    // CoreGraphics uses a top-left origin; AppKit uses bottom-left. Match the
    // actual display and map into its frame, including scaled/negative layouts.
    for (NSScreen *screen in [NSScreen screens]) {
      CGDirectDisplayID display = [[[screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];
      CGRect bounds = CGDisplayBounds(display);
      if (!CGRectContainsPoint(bounds, CGPointMake(x, y)))
        continue;
      NSRect frame = [screen frame];
      NSPoint point = NSMakePoint(NSMinX(frame) + (x - bounds.origin.x) * frame.size.width / bounds.size.width,
                                 NSMaxY(frame) - (y - bounds.origin.y) * frame.size.height / bounds.size.height);
      entryFlash = entryWindow(frame);
      [entryFlash setBackgroundColor:[NSColor colorWithCalibratedWhite:1 alpha:0.08]];
      [entryFlash orderFrontRegardless];
      // Keep the true entry point as the ring center, even at the screen edge.
      NSRect markerFrame = NSMakeRect(point.x - 28, point.y - 28, 56, 56);
      entryMarker = entryWindow(markerFrame);
      DeskflowEntryRing *view = [[DeskflowEntryRing alloc] initWithFrame:NSMakeRect(0, 0, 56, 56)];
      [entryMarker setContentView:view];
      [view release];
      [entryMarker orderFrontRegardless];
      [entryMarker display];
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 80 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        if (generation == entryGeneration)
          [entryFlash orderOut:nil];
      });
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 450 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        if (generation == entryGeneration)
          clearEntryCue();
      });
      break;
    }
  });
}

void runCocoaApp()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  [NSApplication sharedApplication];

  NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 3, 3)
                                                 styleMask:NSBorderlessWindowMask
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
  [window setTitle:@""];
  [window setAlphaValue:0.1];
  [window makeKeyAndOrderFront:nil];

  dummyWindow = window;

  NSLog(@"starting cocoa loop");
  [NSApp run];

  NSLog(@"cocoa: release");
  [pool release];
}

void stopCocoaLoop()
{
  [NSApp stop:dummyWindow];
}
