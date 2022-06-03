import Foundation

class App {
  var url: String?
  var rtmp = RTMP()

  convenience init(url: String) {
    self.init()
    self.url = url
  }

  init() {}

  func parse() {}

  func open() {
    RTMP_Init(&rtmp)
    RTMP_SetupURLEx(&rtmp, url!)
    RTMP_EnableWrite(&rtmp)
    RTMP_Connect(&rtmp, nil)
    RTMP_ConnectStream(&rtmp, 0)
    RTMP_CloseEx(&rtmp)
  }

  static func main() {
    RTMP_LogSetLevel(RTMP_LOGALL)
    let version = RTMP_LibVersion()
    print(String(format: "0x%08x", version))

    let app = App(url: "rtmp://live.nonocast.cn/live/1")
    app.parse()
  }
}

App.main()
