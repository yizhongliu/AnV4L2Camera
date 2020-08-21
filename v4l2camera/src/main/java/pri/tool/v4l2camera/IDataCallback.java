package pri.tool.v4l2camera;

public interface IDataCallback {
    void onDataCallback(byte[] data, int dataType, int width, int height);
}
