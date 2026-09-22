package com.p06.quest;

import android.os.Bundle;
import android.os.Process;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.content.ContentValues;
import android.provider.MediaStore;
import android.net.Uri;
import android.util.Log;
import android.view.WindowManager;
import com.unity3d.player.UnityPlayerActivity;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public final class QuestActivity extends UnityPlayerActivity {
    private static native void nativePrepare(QuestActivity activity, String settingsDirectory, int logFd);
    private java.lang.Process logProcess;
    private ParcelFileDescriptor logDescriptor;
    private FileOutputStream logOutput;

    @Override protected String updateUnityCommandLineArguments(String original) {
        return (original == null ? "" : original) + " -force-gles30";
    }

    @Override protected void onCreate(Bundle state) {
        File files = getExternalFilesDir(null);
        if (files == null) files = getFilesDir();
        String runName = "P06Quest-" + new SimpleDateFormat("yyyyMMdd-HHmmss-SSS", Locale.US).format(new Date()) + ".log";
        int nativeFd = -1;
        try {
            ContentValues values = new ContentValues();
            values.put(MediaStore.Downloads.DISPLAY_NAME, runName);
            values.put(MediaStore.Downloads.MIME_TYPE, "text/plain");
            values.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/P06Quest");
            Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
            if (uri == null) throw new java.io.IOException("Downloads insert failed");
            logDescriptor = getContentResolver().openFileDescriptor(uri, "wa");
            if (logDescriptor == null) throw new java.io.IOException("Downloads open failed");
            logOutput = new FileOutputStream(logDescriptor.getFileDescriptor());
            nativeFd = logDescriptor.getFd();
        } catch (Exception e) {
            Log.w("P06Quest", "Public Downloads unavailable; using app-private fallback", e);
            try {
                logDescriptor = ParcelFileDescriptor.open(new File(files, runName), ParcelFileDescriptor.MODE_CREATE | ParcelFileDescriptor.MODE_WRITE_ONLY | ParcelFileDescriptor.MODE_APPEND);
                logOutput = new FileOutputStream(logDescriptor.getFileDescriptor());
                nativeFd = logDescriptor.getFd();
            } catch (Exception fallback) { Log.e("P06Quest", "Cannot create run log", fallback); }
        }
        try {
            if (logOutput != null) logOutput.write(("P06 Quest candidate 0.1.0\nRun: " + runName + "\nDevice: " + Build.MANUFACTURER + " " + Build.MODEL + "\nAndroid: " + Build.VERSION.RELEASE + " / API " + Build.VERSION.SDK_INT + "\nPackage: " + getPackageName() + "\nSettings: " + files.getAbsolutePath() + "\nPublic log target: Downloads/P06Quest\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
        } catch (Exception e) { Log.e("P06Quest", "Log header failed", e); }
        // Capture this app's own diagnostic stream for sideload-only testing.
        Thread logger = new Thread(() -> {
            try {
                logProcess = new ProcessBuilder("logcat", "-v", "threadtime", "--pid=" + Process.myPid(), "P06Quest:I", "Unity:I", "OpenXR-Loader:I", "AndroidRuntime:E", "*:S").redirectErrorStream(true).start();
                try (InputStream in = logProcess.getInputStream()) {
                    byte[] buffer = new byte[4096]; int n;
                    while ((n = in.read(buffer)) != -1) { if (logOutput != null) { logOutput.write(buffer, 0, n); logOutput.flush(); } }
                }
            } catch (Exception e) { Log.e("P06Quest", "Diagnostic capture unavailable", e); }
        }, "P06 diagnostics");
        logger.setDaemon(true); logger.start();
        System.loadLibrary("openxr_loader");
        System.loadLibrary("p06quest");
        nativePrepare(this, files.getAbsolutePath(), nativeFd);
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override protected void onDestroy() {
        if (logProcess != null) logProcess.destroy();
        super.onDestroy();
    }
}
