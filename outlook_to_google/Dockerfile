# DOCKER_BUILDKIT=1 docker build . --tag=outlook_gmail_sync:local
FROM python:3-alpine

RUN apk add tzdata
RUN cp /usr/share/zoneinfo/Europe/Amsterdam  /etc/localtime
RUN echo "Europe/Amsterdam" >  /etc/timezone
RUN apk del tzdata

WORKDIR /usr/src/app


COPY pyproject.toml .

RUN pip install pdm
COPY ./pdm.lock .
RUN pdm export --without-hashes -o requirements.txt --prod
RUN pip install -r requirements.txt

COPY . .
ADD ./credentials /app/

CMD [ "python", "outlook_to_google.py" ]