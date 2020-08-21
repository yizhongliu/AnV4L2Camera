package pri.tool.v4l2camera;

public interface IStateCallback {

    public void onOpened();

    public void onError(int error);

}
