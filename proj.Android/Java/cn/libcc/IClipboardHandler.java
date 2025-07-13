package cn.libcc;

public interface IClipboardHandler {
    public boolean clipboardHasText();
    public String clipboardGetText();
    public void clipboardSetText(String string);
}
