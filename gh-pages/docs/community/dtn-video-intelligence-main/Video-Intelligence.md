# Using GCP Video Intelligence over the Interplanetary Internet
This project has been developed by Dr Lara Suzuki, a visiting researcher at NASA JPL.


In this tutorial I will demonstrate how to connect use GCP Video Intelligence over the Interplanetary Internet. 

# Setting up the Interplanetary Internet

Please follow tutorial [Multicasting over the Interplanetary Internet](../dtn-multicasting-main/Multicasting-over-ION.md) to set up your nodes. 

***As noted in the multicasting tutorial, current ION `imc` multicasting operation has been updated and adaptation is required to make this tutorial work.***

Once the hosts are configured you can run the command to get ionstart running:

```
$ ./execute
```

# Google Cloud Video Intelligence

Cloud Video Intelligence API allows you to process frames of videos with a simple API call  from anywhere. With GCP Video Intelligence you can:

1. Quickly understand video content by encapsulating powerful machine learning models in an easy to use REST API. 

2. Accurately annotate videos stored in Google Cloud Storage with video and frame-level (1 fps) contextual information. 

3. Make sense of large amount of video files in a very short amount of time.

4. Utilise the technology via an easy to use REST API to analyze videos stored anywhere, or integrate with your image storage on Google Cloud Storage. 

# Executing Google Cloud Video Intelligence

In the Interplanetary host you choose to run the excecution of the Video Intelligence API, install the library:

```
$ pip install --upgrade google-cloud-videointelligence
```

To use the Video Intelligence API you must be Authenticated. To do that, please follow the instructions in this [GCP Tutorial](https://cloud.google.com/docs/authentication/getting-started). After you've created your service account, Provide authentication credentials to your application code by setting the environment variable GOOGLE_APPLICATION_CREDENTIALS.

```
$ export GOOGLE_APPLICATION_CREDENTIALS="/home/user/Downloads/my-key.json"
```

On the host that will run the Video Intelligence, execute: 

```
python
python3 video_processing.py
```
