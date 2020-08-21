package pri.tool.bean;

import java.util.ArrayList;

public class Parameter {
    public int pixFormat;
    public ArrayList<Frame> frames;

    Parameter(int pixFormat, ArrayList<Frame> frames) {
        this.pixFormat = pixFormat;
        this.frames = frames;
    }
}
