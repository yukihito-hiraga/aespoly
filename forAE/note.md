# Note for Evaluators

## Instructions to obtain our artifact

After you login to EC2 account, please download our artifact from https://github.com/yukihito-hiraga/aespoly.git (this is what install.sh do).
Then, you can use our artifact.
See README.

## Instance type and Microarchtecture

You can check the instance type by executing following command on EC2 instance we provide.
```
TOKEN=`curl -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 21600"` \
    && curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/dynamic/instance-identity/document
```

The output should contain ``"instanceType" : "r6i.xlarge"``.

## Anonymity limit

When you login the instance, access log will be created.
This log contains your IP address.
You can anonymize IP address by using VPN.