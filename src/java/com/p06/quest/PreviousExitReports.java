package com.p06.quest;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.os.Build;
import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

/** Recover this package's OS-saved exit evidence before starting Unity again. */
final class PreviousExitReports {
    private static void line(FileOutputStream out, String text) throws java.io.IOException {
        out.write((text + "\n").getBytes(StandardCharsets.UTF_8));
    }
    static void capture(Context context, FileOutputStream out) {
        if (out == null || Build.VERSION.SDK_INT < 30) return;
        try {
            ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            if (manager == null) { line(out, "Previous exit history unavailable: no ActivityManager"); return; }
            java.util.List<ApplicationExitInfo> exits = manager.getHistoricalProcessExitReasons(context.getPackageName(), 0, 3);
            line(out, "Previous process exits: " + exits.size());
            for (ApplicationExitInfo exit : exits) {
                line(out, "Previous exit: timestamp=" + exit.getTimestamp() + " pid=" + exit.getPid()
                    + " process=" + exit.getProcessName() + " reason=" + exit.getReason()
                    + " status=" + exit.getStatus() + " description=" + exit.getDescription());
                try (InputStream trace = exit.getTraceInputStream()) {
                    if (trace == null) { line(out, "Previous exit trace: unavailable"); continue; }
                    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                    byte[] buffer = new byte[4096]; int n;
                    final int limit = 2 * 1024 * 1024;
                    while (bytes.size() < limit && (n = trace.read(buffer, 0, Math.min(buffer.length, limit - bytes.size()))) > 0) bytes.write(buffer, 0, n);
                    boolean truncated = bytes.size() == limit && trace.read() != -1;
                    line(out, "BEGIN PREVIOUS EXIT TRACE BASE64 timestamp=" + exit.getTimestamp() + " truncated=" + truncated);
                    // Native traces on API 31+ are protobuf. Keep bytes intact for offline decoding.
                    line(out, Base64.encodeToString(bytes.toByteArray(), Base64.DEFAULT));
                    line(out, "END PREVIOUS EXIT TRACE BASE64");
                } catch (Exception failure) { line(out, "Previous exit trace error: " + failure); }
            }
            out.flush(); out.getFD().sync();
        } catch (Exception failure) {
            try { line(out, "Previous exit history error: " + failure); } catch (Exception ignored) { }
        }
    }
}
