which pkg-config 1>/dev/null
if [ $? -eq 0 ]; then
    echo "pkg-config installed, continuing..."
else
    rm -rf /var/lib/apt/lists/*
    apt-key --keyserver keyserver.ubuntu.com --recv-keys KEY
    apt-get update
    apt-get install pkg-config
fi

pkg-config glib-2.0
if [ $? -eq 0 ]; then
    echo "glib installed, continuing..."
else
    apt-get install libglib2.0-dev
fi