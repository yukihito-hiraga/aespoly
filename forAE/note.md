# Notes for Evaluators

## Instructions for obtaining our artifact

After you log in to the EC2 instance, please download our artifact from https://github.com/yukihito-hiraga/aespoly.git (this is what ``install.sh`` does).
You can then use our artifact.
Please see the README for details.

## Instance type and Microarchitecture

You can check the instance type by running the following command on the EC2 instance we provide.
```
TOKEN=`curl -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 21600"` \
    && curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/dynamic/instance-identity/document
```

The output should contain ``"instanceType" : "r6i.xlarge"`` which means the instance uses the Ice Lake microarchitecture.

## Before logging out

Since other evaluators may use the same machine, please remove ``aespoly`` folder you downloaded before logging out.
```
$ deactivate
$ cd ~
$ rm -rf aespoly
```