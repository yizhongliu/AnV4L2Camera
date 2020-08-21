package pri.tool.bean;

import java.util.ArrayList;

public class Frame {
    public int width;
    public int height;
    public ArrayList<FrameRate> frameRate;

    Frame(int width, int height, ArrayList<FrameRate> frameRate) {
        this.width = width;
        this.height = height;
        this.frameRate = frameRate;
    }
}
