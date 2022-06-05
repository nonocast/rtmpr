import AVFoundation
import CoreMedia
import Foundation
import SwiftUI
import VideoToolbox

class ImageSource: Source {
  var url: URL?
  var width: Int?
  var height: Int?
  var image: NSImage?
  var pixelBuffer: CVPixelBuffer?
  var encoder: Encoder?
  var timer: Timer?

  init(url: URL) {
    self.url = url
    image = NSImage(byReferencing: url)
    pixelBuffer = image!.colorPixelBuffer()
    print(pixelBuffer!.pixelFormatName())
    print(pixelBuffer!.width, pixelBuffer!.height)
    width = pixelBuffer!.width
    height = pixelBuffer!.height
  }

  func start() {
    timer = Timer.scheduledTimer(withTimeInterval: CMTime(value: 1, timescale: 10).seconds, repeats: true) { _ in
      print("\(#file):\(#line) \(#function)")
      self.encoder!.encode(self.pixelBuffer!)
    }
  }

  func stop() {
    timer!.invalidate()
  }
}

class CameraSource: NSObject, Source {
  var encoder: Encoder?
  var width: Int?
  var height: Int?

  let session = AVCaptureSession()
  private let queue = DispatchQueue(label: "cn.nonocast.camera")

  override init() {
    super.init()
    width = 1280
    height = 768
  }

  func start() {
    guard let device = chooseCaptureDevice() else {
      // throw AppError.CameraNotFound
      return
    }

    guard let videoInput = try? AVCaptureDeviceInput(device: device), session.canAddInput(videoInput) else {
      // throw AppError.CameraOpenError
      return
    }

    session.addInput(videoInput)

    let videoOutput = AVCaptureVideoDataOutput()
    videoOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange]
    videoOutput.setSampleBufferDelegate(self, queue: queue)
    guard session.canAddOutput(videoOutput) else {
      // throw AppError.CameraOpenError
      return
    }

    session.addOutput(videoOutput)
    session.startRunning()
  }

  func stop() {
    session.stopRunning()
  }
}

extension CameraSource: AVCaptureVideoDataOutputSampleBufferDelegate {
  func captureOutput(_: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from _: AVCaptureConnection) {
    encoder!.encode(sampleBuffer.imageBuffer!)
  }
}

extension CameraSource {
  private func chooseCaptureDevice() -> AVCaptureDevice? {
    /*
     under 10.15
     let devices = AVCaptureDevice.devices(for: AVMediaType.video)
     return devices[1]
     */
    let discoverySession = AVCaptureDevice.DiscoverySession(deviceTypes: [.externalUnknown], mediaType: .video, position: .unspecified)
    print("found \(discoverySession.devices.count) device(s)")

    let devices = discoverySession.devices
    guard !devices.isEmpty else { fatalError("found device FAILED") }

    // log all devices
    for each in discoverySession.devices {
      print("- \(each.localizedName)")
    }

    // choose the best
    /*
     obs-virtual-camera 报错时，需要去掉codesign
     https://obsproject.com/wiki/MacOS-Virtual-Camera-Compatibility-Guide
     sudo codesign --remove-signature CameraApp.app
     sudo codesign --sign - Camera.app
     */

    return devices[3]
  }
}
