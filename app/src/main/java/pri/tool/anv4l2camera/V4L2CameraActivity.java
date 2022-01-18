package pri.tool.anv4l2camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.ExpandableListView;
import android.widget.LinearLayout;
import android.widget.Toast;


import androidx.core.app.ActivityCompat;

import java.util.ArrayList;
import java.util.Map;

import pri.tool.bean.Parameter;
import pri.tool.v4l2camera.IDataCallback;
import pri.tool.v4l2camera.IStateCallback;
import pri.tool.v4l2camera.V4L2Camera;

import static pri.tool.v4l2camera.ImageUtils.H264;
import static pri.tool.v4l2camera.ImageUtils.MJPEG;
import static pri.tool.v4l2camera.ImageUtils.YUYV;

public class V4L2CameraActivity extends Activity {
    private final static String TAG = "V4L2CameraActivity";

    V4L2Camera adCamera;
    CameraStateCallback cameraStateCallback;
    CameraDataCallback cameraDataCallback;
    ArrayList<Parameter> parameters;

    AutoFitSurfaceView surfaceView;
    SurfaceHolder surfaceHolder;

    private int previewWidth = 1920;
    private int previewHeight = 1080;

    String[] pixformats;
    String[][] resolutions;

    ExpandableListView expandableListView;
    MyExpandableListAdapter adapter;

    int pixClick = -1;
    int resolutionClick = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        verifyStoragePermissions(this);

        initView();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    public void initView() {
        surfaceView = findViewById(R.id.cameraSurface);

        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder surfaceHolder) {
                Log.e(TAG, "Surface create");
                initCamera();
            }

            @Override
            public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

            }
        });

    }



    public void initCamera() {
        cameraStateCallback = new CameraStateCallback();
        adCamera = new V4L2Camera();
        adCamera.init(cameraStateCallback, this);
        adCamera.open();
    }

    class CameraStateCallback implements IStateCallback {

        @Override
        public void onOpened() {
            Log.d(TAG, "onOpened");

            parameters = adCamera.getCameraParameters();

            pixformats = new String[parameters.size()];
            resolutions = new String[parameters.size()][];

            String sPixFormat;
            for (int i = 0; i < parameters.size(); i++) {
                Log.e(TAG, "format:" + parameters.get(i).pixFormat);
                switch (parameters.get(i).pixFormat) {
                    case YUYV:
                        sPixFormat = "YUYV";
                        break;
                    case MJPEG:
                        sPixFormat = "MJPEG";
                        break;
                    case H264:
                        sPixFormat = "H264";
                        break;
                    default:
                        sPixFormat = "UNKNOW";
                        break;
                }

                pixformats[i] = sPixFormat;

                int resolutionSize = parameters.get(i).frames.size();

                resolutions[i] = new String[resolutionSize];

                for (int j = 0; j < resolutionSize; j++) {
                    String resolution = parameters.get(i).frames.get(j).width + "*" + parameters.get(i).frames.get(j).height;
                    resolutions[i][j] = resolution;
                }
            }

            showDialog();
        }

        @Override
        public void onError(int error) {

        }
    }

    class CameraDataCallback implements IDataCallback {

        @Override
        public void onDataCallback(byte[] data, int dataType, int width, int height) {
            Log.e(TAG, "onDataCallbakck  dataType:" + dataType + ", width:" + width + ", height:" + height);
            //处理camera preview 数据
        }
    }

    private void showDialog() {
        LinearLayout linearLayoutMain = new LinearLayout(this);//自定义一个布局文件
        linearLayoutMain.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        expandableListView = new ExpandableListView(this);

        //自定义 展开/收起  图标的点击事件。position和 isExpand 都是通过tag 传递的
        View.OnClickListener ivGoToChildClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //获取被点击图标所在的group的索引
                Map<String, Object> map = (Map<String, Object>) v.getTag();
                int groupPosition = (int) map.get("groupPosition");
//                boolean isExpand = (boolean) map.get("isExpanded");   //这种是通过tag传值
                boolean isExpand = expandableListView.isGroupExpanded(groupPosition);    //判断分组是否展开

                if (isExpand) {
                    expandableListView.collapseGroup(groupPosition);
                } else {
                    expandableListView.expandGroup(groupPosition);
                }
            }
        };

        //创建并设置适配器
        adapter = new MyExpandableListAdapter(pixformats, resolutions, V4L2CameraActivity.this,
                ivGoToChildClickListener);
        expandableListView.setAdapter(adapter);

        //默认展开第一个分组
        expandableListView.expandGroup(0);

        //展开某个分组时，并关闭其他分组。注意这里设置的是 ExpandListener
        expandableListView.setOnGroupExpandListener(new ExpandableListView.OnGroupExpandListener() {
            @Override
            public void onGroupExpand(int groupPosition) {
                //遍历 group 的数组（或集合），判断当前被点击的位置与哪个组索引一致，不一致就合并起来。
                for (int i = 0; i < pixformats.length; i++) {
                    if (i != groupPosition) {
                        expandableListView.collapseGroup(i); //收起某个指定的组
                    }
                }
            }
        });

        //点击某个分组时，跳转到指定Activity
        expandableListView.setOnGroupClickListener(new ExpandableListView.OnGroupClickListener() {
            @Override
            public boolean onGroupClick(ExpandableListView parent, View v, int groupPosition, long id) {
                Toast.makeText(V4L2CameraActivity.this, "组被点击了，跳转到具体的Activity", Toast.LENGTH_SHORT).show();
                return true;    //拦截点击事件，不再处理展开或者收起
            }
        });

        //某个分组中的子View被点击时的事件
        expandableListView.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
            @Override
            public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition,
                                        long id) {

                Log.e(TAG, "onChildClick (" + groupPosition + "," + childPosition + ")");

                int gourpsSum = adapter.getGroupCount();//组的数量
                for(int i = 0; i < gourpsSum; i++) {
                    int childSum = adapter.getChildrenCount(i);//组中子项的数量
                    for(int k = 0; k < childSum;k++) {
                        boolean isLast = false;
                        if (k == (childSum - 1)){
                            isLast = true;
                        }

                        CheckBox cBox = (CheckBox) adapter.getChildView(i, k, isLast, null, null).findViewById(R.id.cb_elvChild);
                        cBox.toggle();//切换CheckBox状态！！！！！！！！！！
                        boolean itemIsCheck=cBox.isChecked();

                        Log.e(TAG, "(" + i + "," + k + ") :" + itemIsCheck );

                        if (i == groupPosition && k == childPosition) {
                            adapter.setChildSelectState(pixformats[i] + resolutions[i][k], itemIsCheck);
                            if (itemIsCheck) {
                                pixClick = groupPosition;
                                resolutionClick = childPosition;
                            } else {
                                pixClick = -1;
                                resolutionClick = -1;
                            }
                        } else {
                            adapter.setChildSelectState(pixformats[i] + resolutions[i][k], false);
                        }

                        ((BaseExpandableListAdapter) adapter).notifyDataSetChanged();//通知数据发生了变化
                    }

                }
                return true;
            }
        });


        linearLayoutMain.addView(expandableListView);//往这个布局中加入listview

        final AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("选择图像格式和分辨率").setView(linearLayoutMain)//在这里把写好的这个listview的布局加载dialog中
                .setNegativeButton("取消", new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // TODO Auto-generated method stub
                        dialog.cancel();
                    }
                })
                .setPositiveButton("确定", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (pixClick == -1 && resolutionClick == -1) {
                            Toast.makeText(V4L2CameraActivity.this, "请选择图像格式和分辨率", Toast.LENGTH_SHORT).show();
                        } else {
                            previewWidth = parameters.get(pixClick).frames.get(resolutionClick).width;
                            previewHeight = parameters.get(pixClick).frames.get(resolutionClick).height;

                            int ret;
                            ret = adCamera.setPreviewParameter(previewWidth, previewHeight, parameters.get(pixClick).pixFormat);
                            if (ret < 0) {
                                Toast.makeText(V4L2CameraActivity.this, "不支持该图像格式", Toast.LENGTH_SHORT).show();
                                return;
                            }

                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    surfaceView.setAspectRatio(previewWidth, previewHeight);
                                }
                            });

                            adCamera.setSurface(surfaceHolder);

                            cameraDataCallback = new CameraDataCallback();
                            adCamera.startPreview(cameraDataCallback);

                            dialog.cancel();

                        }
                    }
                }).create();
        dialog.setCanceledOnTouchOutside(false);//使除了dialog以外的地方不能被点击
        dialog.show();

    }

    private static final int REQUEST_EXTERNAL_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {
            "android.permission.READ_EXTERNAL_STORAGE",
            "android.permission.WRITE_EXTERNAL_STORAGE" };
    /*
     * android 动态权限申请
     * */
    public static void verifyStoragePermissions(Activity activity) {
        try {
            //检测是否有写的权限
            int permission = ActivityCompat.checkSelfPermission(activity,
                    "android.permission.WRITE_EXTERNAL_STORAGE");
            if (permission != PackageManager.PERMISSION_GRANTED) {
                // 没有写的权限，去申请写的权限，会弹出对话框
                ActivityCompat.requestPermissions(activity, PERMISSIONS_STORAGE,REQUEST_EXTERNAL_STORAGE);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
