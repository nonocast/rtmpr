import CoreMedia
import Foundation
import SwiftUI

protocol Source {
  var width: Int? { get }
  var height: Int? { get }
  var encoder: Encoder? { get set }
  func start()
  func stop()
}

protocol Output {
  func open()
  func close()
  func onVideoMetadata(sps: Data, pps: Data)
  func onVideoSample(_ sampleBuffer: CMSampleBuffer)
}

protocol Encoder {
  var output: Output? { get set }
  func open()
  func close()
  func encode(_ imageBuffer: CVImageBuffer)
}

enum AppError: Error {
  case CameraNotFound
  case CameraOpenError
  case ScreenNotFound
  case ScreenOpenError
}

@main
class App {
  static let shared = App()
  var session: Session?

  init() {
    print("app running...")
  }

  func run() {
    session = Session()
    // session!.bindSource(ImageSource(url: URL(fileURLWithPath: "assets/sample.png")))
    session!.bindSource(CameraSource())
    session!.bindOutput(RTMPDelegate(url: URL(string: "rtmp://shgbit.xyz/foo/1")))
    // session!.bindOutput(H264FileDelegate(url: URL(fileURLWithPath: "out.h264"))
    session!.start()
  }

  func close() {
    session!.stop()
    exit(0)
  }

  static func main() {
    signal(SIGINT) { _ in App.shared.close() }
    App.shared.run()
    RunLoop.main.run()
  }
}
