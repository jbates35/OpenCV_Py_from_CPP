import tensorflow as tf
import os
import numpy as np

cwd = os.getcwd()
model_rel_path = 'models/fishModel1'
model_abs_path = os.path.join(cwd, model_rel_path)

IMAGE_SIZE = 512
fish_dir = os.path.join(cwd, 'savedfish')

model = tf.saved_model.load(model_rel_path)

# A generator that provides a representative dataset
def representative_data_gen():
  dataset_list = tf.data.Dataset.list_files(fish_dir + '/*')
  for i in range(100):
    image = next(iter(dataset_list))
    image = tf.io.read_file(image)
    image = tf.io.decode_jpeg(image, channels=3)
    image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE])
    image = tf.cast(image / 255., tf.float32)
    image = tf.expand_dims(image, 0)
    yield [image]

converter = tf.lite.TFLiteConverter.from_saved_model(model_rel_path)
# This enables quantization
converter.optimizations = [tf.lite.Optimize.DEFAULT]
# This sets the representative dataset for quantization
converter.representative_dataset = representative_data_gen
# This ensures that if any ops can't be quantized, the converter throws an error
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8, tf.lite.OpsSet.SELECT_TF_OPS]
# Disabling _experimental_lower_tensor_list_ops
converter._experimental_lower_tensor_list_ops = False

# Set allow_custom_ops to True to disable the conversion of unsupported ops
converter.allow_custom_ops = True
# For full integer quantization, though supported types defaults to int8 only, we explicitly declare it for clarity.
converter.target_spec.supported_types = [tf.int8]
# These set the input and output tensors to uint8 (added in r2.3)
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8
tflite_model = converter.convert()

# Save TFLite model
with open('converted_model3.tflite', 'wb') as f:
    f.write(tflite_model)
