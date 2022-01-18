package pri.tool.anv4l2camera;

import android.content.Context;
import android.nfc.Tag;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.HashMap;
import java.util.Map;

public class MyExpandableListAdapter extends BaseExpandableListAdapter {
    public final static String TAG = "MyExpandableListAdapter";

    private String[]   pixformat;
    private String[][] resolution;
    private Context context;
    View.OnClickListener ivGoToChildClickListener;

    HashMap<String, Boolean> statusHashMap;

    CheckBox childBox;

    public MyExpandableListAdapter() {

    }

    public MyExpandableListAdapter(String[] pixformat, String[][] resolution, Context context,
                                   View.OnClickListener ivGoToChildClickListener) {
        this.pixformat = pixformat;
        this.resolution = resolution;
        this.context = context;
        this.ivGoToChildClickListener = ivGoToChildClickListener;

        statusHashMap = new HashMap<String, Boolean>();
        for (int i = 0; i < resolution.length; i++) {// 初始时,让所有的子选项均未被选中
            for (int a = 0; a < resolution[i].length; a++) {
                statusHashMap.put(pixformat[i] + resolution[i][a], false);
            }
        }
    }

    @Override
    public int getGroupCount() {    //组的数量
        return pixformat.length;
    }

    @Override
    public int getChildrenCount(int groupPosition) {    //某组中子项数量
        return resolution[groupPosition].length;
    }

    @Override
    public Object getGroup(int groupPosition) {     //某组
        return pixformat[groupPosition];
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {  //某子项
        return resolution[groupPosition][childPosition];
    }

    @Override
    public long getGroupId(int groupPosition) {
        return groupPosition;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return childPosition;
    }


    @Override
    public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent) {
        GroupHold groupHold;
        if (convertView == null) {
            convertView = LayoutInflater.from(context).inflate(R.layout.item_elv_group, null);
            groupHold = new GroupHold();
            groupHold.tvGroupName = (TextView) convertView.findViewById(R.id.tv_groupName);
            groupHold.ivGoToChildLv = (ImageView) convertView.findViewById(R.id.iv_goToChildLV);

            convertView.setTag(groupHold);

        } else {
            groupHold = (GroupHold) convertView.getTag();

        }

        String groupName = pixformat[groupPosition];
        groupHold.tvGroupName.setText(groupName);


        //取消默认的groupIndicator后根据方法中传入的isExpand判断组是否展开并动态自定义指示器
//        if (isExpanded) {   //如果组展开
//            groupHold.ivGoToChildLv.setImageResource(R.drawable.delete);
//        } else {
//            groupHold.ivGoToChildLv.setImageResource(R.drawable.send_btn_add);
//        }

        //setTag() 方法接收的类型是object ，所以可将position和converView先封装在Map中。Bundle中无法封装view,所以不用bundle
        Map<String, Object> tagMap = new HashMap<>();
        tagMap.put("groupPosition", groupPosition);
        tagMap.put("isExpanded", isExpanded);
        groupHold.ivGoToChildLv.setTag(tagMap);

        //图标的点击事件
        groupHold.ivGoToChildLv.setOnClickListener(ivGoToChildClickListener);

        return convertView;
    }

    @Override
    public View getChildView(final int groupPosition, final int childPosition, boolean isLastChild, View convertView,
                             ViewGroup parent) {
        ChildHold childHold;
        if (convertView == null) {
            convertView = LayoutInflater.from(context).inflate(R.layout.item_elv_child, null);
            childHold = new ChildHold();
            childHold.tvChildName = (TextView) convertView.findViewById(R.id.tv_elv_childName);
            childHold.cbElvChild = (CheckBox) convertView.findViewById(R.id.cb_elvChild);
            convertView.setTag(childHold);
        } else {
            childHold = (ChildHold) convertView.getTag();
        }

        Log.e(TAG, "getChildView");

        String childName = resolution[groupPosition][childPosition];
        childHold.tvChildName.setText(childName);

        Boolean nowStatus = statusHashMap.get(pixformat[groupPosition] + resolution[groupPosition][childPosition]);//当前状态
        childHold.cbElvChild.setChecked(nowStatus);

//        childHold.cbElvChild.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
//            @Override
//            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
//                if (isChecked) {
//                    Toast.makeText(context, "条目选中:" + pixformat[groupPosition] + "的" +
//                            resolution[groupPosition][childPosition] + "被选中了", Toast.LENGTH_SHORT).show();
//                } else {
//                    Toast.makeText(context, "条目选中:" + pixformat[groupPosition] + "的" +
//                            resolution[groupPosition][childPosition] + "被取消选中了", Toast.LENGTH_SHORT).show();
//                }
//            }
//        });

        return convertView;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;    //默认返回false,改成true表示组中的子条目可以被点击选中
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

    public void setChildSelectState(String tag, boolean itemSelect) {
        statusHashMap.put(tag, itemSelect);
    }

    class GroupHold {
        TextView  tvGroupName;
        ImageView ivGoToChildLv;
    }

    class ChildHold {
        TextView tvChildName;
        CheckBox cbElvChild;
    }
}
