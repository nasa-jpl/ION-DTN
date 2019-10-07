package gov.nasa.jpl.camerashare;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.support.v4.content.FileProvider;
import android.support.v7.widget.RecyclerView;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;

import java.io.File;
import java.util.ArrayList;

public class GalleryRecyclerViewAdapter extends RecyclerView
        .Adapter<GalleryRecyclerViewAdapter.CustomViewHolder>{
    private ArrayList<File> picList;
    private static final String TAG = "GalleryRecyclerView";
    private Context actcontext;

    /**
     * Constructor used for initializing the view generation process
     */
    public static class CustomViewHolder extends RecyclerView.ViewHolder
            implements View.OnCreateContextMenuListener{
        public View my_view;
        public ViewGroup parent;
        public CustomViewHolder(View v, ViewGroup parent) {
            super(v);
            my_view = v;
            this.parent = parent;
            v.setOnCreateContextMenuListener(this);
        }

        @Override
        public void onCreateContextMenu(ContextMenu contextMenu, View view,
                                        ContextMenu.ContextMenuInfo contextMenuInfo) {
            contextMenu.setHeaderTitle("Select Action");
            contextMenu.add(0, 0, getAdapterPosition(), "Open");
            contextMenu.add(0, 1, getAdapterPosition(), "Delete");
        }
    }

    public GalleryRecyclerViewAdapter(ArrayList<File> list, Context fr) {
        picList = list;
        this.actcontext = fr;
    }

    /**
     * Creates new view objects
     * @param parent The parent view group of the new item view object
     * @param viewType Not used but required
     * @return A new view object
     */
    @Override
    public GalleryRecyclerViewAdapter.CustomViewHolder onCreateViewHolder(final
                                                                          ViewGroup parent,
                                                                          int viewType) {
        // create a new view
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.gallery_recycler_view_item_layout, parent,
                        false);
        // set the view's size, margins, paddings and layout parameters
        return new CustomViewHolder(v, parent);
    }

    /**
     * Replaces an view's content with new content
     * @param holder The view object
     * @param position The position of the object within the dataset
     */
    @Override
    public void onBindViewHolder(final CustomViewHolder holder, final int position) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element

        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent();
                intent.setAction(Intent.ACTION_VIEW);
                Uri photoURI = FileProvider.getUriForFile(actcontext,
                        "com.example.android.fileprovider",
                        picList.get(position));
                intent.setDataAndType(photoURI, "image/*").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                actcontext.startActivity(intent);
            }
        });

        ImageView image = holder.itemView.findViewById(R.id
                .img);

        File pic = picList.get(position);
        image.setScaleType(ImageView.ScaleType.CENTER_CROP);

        Glide.with(actcontext)
                .load(pic)
                .into(image);
    }

    /**
     * Get the number of items in the dataset
     * @return The number of items as integer
     */
    @Override
    public int getItemCount() {
        return picList.size();
    }
}
