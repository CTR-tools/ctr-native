package com.ctrnative;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;

public final class CTRNativeLauncherActivity extends Activity {
    private static final int REQUEST_DISC_IMAGE = 1001;
    private static final int COPY_BUFFER_SIZE = 1024 * 1024;
    private static final int RAW_SECTOR_SIZE = 2352;
    private static final int PVD_LBA = 16;
    private static final int FORM1_DATA_OFFSET = 24;

    private Button importButton;
    private ProgressBar importProgress;
    private TextView statusText;
    private boolean importInProgress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        File discImage = getImportedDiscImage();
        if (isRetailDiscImage(discImage)) {
            launchGame();
            return;
        }

        buildImportView(discImage.exists());
    }

    private File getStorageRoot() {
        File root = getExternalFilesDir(null);
        return (root != null) ? root : getFilesDir();
    }

    private File getImportedDiscImage() {
        return new File(new File(getStorageRoot(), "assets"), "ctr-u.bin");
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private TextView makeText(String text, float size, int color) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(size);
        view.setGravity(Gravity.CENTER);
        view.setMaxWidth(dp(720));
        return view;
    }

    private void buildImportView(boolean invalidExistingImage) {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(dp(32), dp(24), dp(32), dp(24));
        root.setBackgroundColor(Color.rgb(18, 18, 18));

        TextView title = makeText(getString(R.string.setup_title), 30.0f, Color.WHITE);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        titleParams.bottomMargin = dp(18);
        root.addView(title, titleParams);

        TextView instructions = makeText(
                getString(R.string.setup_instructions),
                17.0f,
                Color.LTGRAY);
        LinearLayout.LayoutParams instructionsParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        instructionsParams.bottomMargin = dp(20);
        root.addView(instructions, instructionsParams);

        statusText = makeText(
                getString(invalidExistingImage ? R.string.setup_invalid_disc : R.string.setup_no_disc),
                15.0f,
                invalidExistingImage ? Color.rgb(255, 160, 122) : Color.LTGRAY);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        statusParams.bottomMargin = dp(16);
        root.addView(statusText, statusParams);

        importProgress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        importProgress.setMax(100);
        importProgress.setVisibility(View.GONE);
        LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(dp(420), dp(12));
        progressParams.bottomMargin = dp(18);
        root.addView(importProgress, progressParams);

        importButton = new Button(this);
        importButton.setText(invalidExistingImage ? R.string.setup_replace_disc : R.string.setup_select_disc);
        importButton.setOnClickListener(view -> selectDiscImage());
        root.addView(importButton);

        setContentView(root);
    }

    private void selectDiscImage() {
        if (importInProgress) {
            return;
        }

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_DISC_IMAGE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if ((requestCode != REQUEST_DISC_IMAGE) || (resultCode != RESULT_OK) || (data == null) || (data.getData() == null)) {
            return;
        }

        importDiscImage(data.getData());
    }

    private long querySourceSize(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, new String[] {OpenableColumns.SIZE}, null, null, null)) {
            if ((cursor != null) && cursor.moveToFirst() && !cursor.isNull(0)) {
                return cursor.getLong(0);
            }
        } catch (Exception exception) {
            return -1;
        }

        return -1;
    }

    private void importDiscImage(Uri uri) {
        importInProgress = true;
        importButton.setEnabled(false);

        long sourceSize = querySourceSize(uri);
        importProgress.setIndeterminate(sourceSize <= 0);
        importProgress.setProgress(0);
        importProgress.setVisibility(View.VISIBLE);
        statusText.setText(R.string.setup_importing_disc);
        statusText.setTextColor(Color.LTGRAY);

        Thread worker = new Thread(() -> copyDiscImage(uri, sourceSize), "CTR disc import");
        worker.start();
    }

    private void copyDiscImage(Uri uri, long sourceSize) {
        File destination = getImportedDiscImage();
        File assetDirectory = destination.getParentFile();
        File temporary = new File(assetDirectory, "ctr-u.bin.importing");

        try {
            if (!assetDirectory.isDirectory() && !assetDirectory.mkdirs()) {
                throw new IOException("Could not create the app asset directory");
            }

            if (temporary.exists() && !temporary.delete()) {
                throw new IOException("Could not replace an incomplete import");
            }

            ContentResolver resolver = getContentResolver();
            try (InputStream input = resolver.openInputStream(uri);
                 FileOutputStream output = new FileOutputStream(temporary)) {
                if (input == null) {
                    throw new IOException("The selected file could not be opened");
                }

                byte[] buffer = new byte[COPY_BUFFER_SIZE];
                long copied = 0;
                int lastProgress = -1;
                int read;

                while ((read = input.read(buffer)) != -1) {
                    if (read == 0) {
                        continue;
                    }

                    output.write(buffer, 0, read);
                    copied += read;

                    if (sourceSize > 0) {
                        int progress = (int)Math.min(100, copied * 100 / sourceSize);
                        if (progress != lastProgress) {
                            lastProgress = progress;
                            int displayProgress = progress;
                            runOnUiThread(() -> updateImportProgress(displayProgress));
                        }
                    }
                }

                output.getFD().sync();
            }

            if (!isRetailDiscImage(temporary)) {
                throw new IOException("The selected file is not a supported raw NTSC-U BIN image");
            }

            if (destination.exists() && !destination.delete()) {
                throw new IOException("Could not replace the previous disc image");
            }

            if (!temporary.renameTo(destination)) {
                throw new IOException("Could not finish the disc image import");
            }

            runOnUiThread(() -> {
                statusText.setText(R.string.setup_starting_game);
                launchGame();
            });
        } catch (Exception exception) {
            temporary.delete();
            String detail = exception.getMessage();
            if ((detail == null) || detail.isEmpty()) {
                detail = exception.getClass().getSimpleName();
            }

            String error = detail;
            runOnUiThread(() -> showImportError(error));
        }
    }

    private void updateImportProgress(int progress) {
        importProgress.setProgress(progress);
        statusText.setText(getString(R.string.setup_import_progress, progress));
    }

    private void showImportError(String error) {
        importInProgress = false;
        importProgress.setVisibility(View.GONE);
        importButton.setEnabled(true);
        importButton.setText(R.string.setup_select_another_disc);
        statusText.setText(error);
        statusText.setTextColor(Color.rgb(255, 160, 122));
    }

    private boolean isRetailDiscImage(File image) {
        if (!image.isFile() || (image.length() < (long)(PVD_LBA + 1) * RAW_SECTOR_SIZE) || ((image.length() % RAW_SECTOR_SIZE) != 0)) {
            return false;
        }

        byte[] sector = new byte[RAW_SECTOR_SIZE];
        try (RandomAccessFile file = new RandomAccessFile(image, "r")) {
            file.seek((long)PVD_LBA * RAW_SECTOR_SIZE);
            file.readFully(sector);
        } catch (IOException exception) {
            return false;
        }

        if ((sector[0] != 0) || (sector[11] != 0) || (sector[15] != 2)) {
            return false;
        }

        for (int i = 1; i < 11; i++) {
            if ((sector[i] & 0xff) != 0xff) {
                return false;
            }
        }

        return (sector[FORM1_DATA_OFFSET] == 1)
                && (sector[FORM1_DATA_OFFSET + 1] == 'C')
                && (sector[FORM1_DATA_OFFSET + 2] == 'D')
                && (sector[FORM1_DATA_OFFSET + 3] == '0')
                && (sector[FORM1_DATA_OFFSET + 4] == '0')
                && (sector[FORM1_DATA_OFFSET + 5] == '1')
                && (sector[FORM1_DATA_OFFSET + 6] == 1);
    }

    private void launchGame() {
        Intent intent = new Intent(this, CTRNativeActivity.class);
        startActivity(intent);
        finish();
    }
}
