import os
import json
import random
import shutil
from collections import defaultdict

# ============================================================
# CONFIG
# ============================================================

ROOT_DIR = "Indoor Obstacle.v4i.coco"

OUTPUT_DIR = "YOLO_Obstacle_Final_lowres"

# Final dataset size
TARGETS = {
    "train": 10000,
    "valid": 2000,
    "test": 2000
}

random.seed(42)

# ============================================================
# HELPERS
# ============================================================

def load_coco(json_path):
    with open(json_path, "r") as f:
        return json.load(f)

def save_yolo_label(txt_path, anns, img_w, img_h):

    with open(txt_path, "w") as f:

        for ann in anns:

            x, y, w, h = ann["bbox"]

            # Convert COCO -> YOLO
            x_center = (x + w / 2) / img_w
            y_center = (y + h / 2) / img_h

            w /= img_w
            h /= img_h

            # Single class = 0
            f.write(
                f"0 {x_center} {y_center} {w} {h}\n"
            )

# ============================================================
# PROCESS
# ============================================================

for split, target_size in TARGETS.items():

    print(f"\nProcessing {split}...")

    split_dir = os.path.join(ROOT_DIR, split)

    json_path = os.path.join(
        split_dir,
        "_annotations.coco.json"
    )

    coco = load_coco(json_path)

    images = coco["images"]
    annotations = coco["annotations"]

    # Map image_id -> annotations
    ann_map = defaultdict(list)

    for ann in annotations:
        ann_map[ann["image_id"]].append(ann)

    # Shuffle images
    random.shuffle(images)

    # Keep only images with annotations
    images = [
        img for img in images
        if len(ann_map[img["id"]]) > 0
    ]

    # Reduce dataset
    selected_images = images[:target_size]

    # ========================================================
    # OUTPUT FOLDERS
    # ========================================================

    out_images = os.path.join(
        OUTPUT_DIR,
        split,
        "images"
    )

    out_labels = os.path.join(
        OUTPUT_DIR,
        split,
        "labels"
    )

    os.makedirs(out_images, exist_ok=True)
    os.makedirs(out_labels, exist_ok=True)

    # ========================================================
    # PROCESS IMAGES
    # ========================================================

    for img in selected_images:

        img_name = img["file_name"]

        src_img = os.path.join(split_dir, img_name)

        dst_img = os.path.join(out_images, img_name)

        if not os.path.exists(src_img):
            continue

        # Copy image
        shutil.copy2(src_img, dst_img)

        # Create YOLO label
        txt_name = os.path.splitext(img_name)[0] + ".txt"

        txt_path = os.path.join(out_labels, txt_name)

        anns = ann_map[img["id"]]

        save_yolo_label(
            txt_path,
            anns,
            img["width"],
            img["height"]
        )

    print(f"{split} done.")
    print(f"Images: {len(selected_images)}")

# ============================================================
# CREATE data.yaml
# ============================================================

yaml_text = """
train: train/images
val: valid/images
test: test/images

nc: 1

names:
  0: obstacle
"""

yaml_path = os.path.join(
    OUTPUT_DIR,
    "data.yaml"
)

with open(yaml_path, "w") as f:
    f.write(yaml_text)

print("\nDONE.")
print(f"Final dataset saved to: {OUTPUT_DIR}")