package org.libcc;

import android.content.Context;

class ClipboardHandler_API implements
    IClipboardHandler,
    android.content.ClipboardManager.OnPrimaryClipChangedListener {

    protected android.content.ClipboardManager mClipMgr;

    ClipboardHandler_API() {
       mClipMgr = (android.content.ClipboardManager) CCWidgets.context.getSystemService(Context.CLIPBOARD_SERVICE);
       mClipMgr.addPrimaryClipChangedListener(this);
    }

    @Override
    public boolean clipboardHasText() {
       return mClipMgr.hasText();
    }

    @Override
    public String clipboardGetText() {
        CharSequence text;
        text = mClipMgr.getText();
        if (text != null) {
           return text.toString();
        }
        return null;
    }

    @Override
    public void clipboardSetText(String string) {
       mClipMgr.removePrimaryClipChangedListener(this);
       mClipMgr.setText(string);
       mClipMgr.addPrimaryClipChangedListener(this);
    }

    @Override
    public void onPrimaryClipChanged() {
    }
}