{
  "targets": [
    {
      "target_name": "netlinknfc",
      "sources": [ "src/nfc.cpp" ],
      "include_dirs": [
        "/usr/include/libnl3"
      ],
      "libraries": [
        "-lnl-3", "-lnl-genl-3"
      ]
    }
  ]
}