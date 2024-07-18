import argparse
from PIL import Image


def convert_tiff_to_tga(input_path, output_path):
    try:
        # TIFF 파일 열기
        tiff_image = Image.open(input_path)

        # TGA 파일로 저장
        tiff_image.save(output_path)

        print(f"'{input_path}' 파일이 '{output_path}' 파일로 성공적으로 변환되었습니다.")
    except Exception as e:
        print(f"파일 변환 중 오류가 발생했습니다: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TIFF 파일을 TGA 파일로 변환하는 스크립트")
    parser.add_argument('input', type=str, help='변환할 TIFF 파일 경로')
    parser.add_argument('output', type=str, help='저장할 TGA 파일 경로')

    args = parser.parse_args()
    convert_tiff_to_tga(args.input, args.output)
