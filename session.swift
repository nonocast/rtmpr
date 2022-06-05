class Session {
  var source: Source?
  var output: Output?
  var encoder: Encoder?

  func bindSource(_ source: Source) {
    print("\(#file):\(#line) \(#function)")

    assert(self.source == nil)
    print(source)
    self.source = source
    encoder = VTH264Encoder(width: self.source!.width!, height: self.source!.height!)
    self.source!.encoder = encoder
  }

  func bindOutput(_ output: Output) {
    print("\(#file):\(#line) \(#function)")

    assert(source != nil)
    assert(encoder != nil)
    assert(self.output == nil)

    self.output = output
    encoder!.output = self.output
  }

  func start() {
    output!.open()
    encoder!.open()
    source!.start()
  }

  func stop() {
    source!.stop()
    encoder!.close()
    output!.close()
  }
}
